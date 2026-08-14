# CMAudio Bus 树 + 分组音量 实现分析报告

> 目标：为 CMAudio 设计音频总线（Bus）树与分组音量控制，支持游戏常见的
> "Master / Music / SFX / Ambient / UI" 音量分级与独立静音。
> 前置调研：逐一分析 AudioSource / AudioPlayer / MIDIPlayer / MIDIOrchestraPlayer
> / AudioManager / SpatialAudioWorld 六条发声路径的增益处理。

---

## 一、现状：六条发声路径的增益语义调研

### 1.1 所有路径的最终汇聚点

```
AudioPlayer        ─┐
MIDIPlayer         ─┤
MIDIOrchestraPlayer─┤
AudioManager       ─┼──→  AudioSource::SetGain(g) ──→ alSourcef(index, AL_GAIN, g)
SpatialAudioWorld  ─┤
（直接使用 AudioSource）
```

关键事实：**六条路径全部收敛到 `AudioSource::SetGain()` 这一个函数**（`AudioSource.cpp:136-143`）。这是 Bus 增益叠加的天然唯一注入点。

### 1.2 各路径增益语义差异（结构合理性评估）

| 类 | 增益语义 | 最终写入 | 结构问题 |
|---|---|---|---|
| `AudioSource` | `gain` 字段 = 基础增益，`SetGain` 直写 OpenAL | `alSourcef(AL_GAIN, gain)` | ✅ 干净，无内部增益计算 |
| `AudioPlayer` | `gain` + `auto_gain` + `FadeFactor` 三者相乘，在**播放线程** `UpdateBuffer` 里计算 | `audiosource.SetGain(factor*gain)` 或 `SetGain(g)` | ⚠️ 增益在**两处**写：`SetGain(val)` 直写 source，但 `UpdateBuffer` 又会覆盖为 `factor*gain` |
| `MIDIPlayer` | 同 AudioPlayer，`gain` + auto_gain + FadeFactor | `audiosource.SetGain(...)` | ⚠️ 同上 |
| `MIDIOrchestraPlayer` | **16 通道各自独立增益**：`channel_positions[ch].gain`，整体 auto_gain 再乘 | `sources[ch]->SetGain(...)` | ⚠️ 增益 = 通道音量 × 全局 auto_gain，多级 |
| `AudioManager` | `Play(filename, gain)` 透传给 `source->SetGain(gain)` | `source->SetGain(gain)` | ✅ 简单透传 |
| `SpatialAudioWorld` | `asi->gain`（源增益）+ **OpenAL 距离衰减自动叠加** + 优先级偷取时 `VOICE_STEAL_GAIN_REDUCTION` | `asi->source->SetGain(asi->gain)` | ⚠️ 距离衰减由 OpenAL 在 AL_GAIN 上自动乘，无法分离 |

### 1.3 结构合理性结论

**核心矛盾：`AudioPlayer`/`MIDIPlayer` 的增益"双写"问题。**

`AudioPlayer::SetGain(val)`（`AudioPlayer.h:106`）直接调 `audiosource.SetGain(val)`，但下一次 `UpdateBuffer()` 里 `audiosource.SetGain(factor*gain)` 会**覆盖**它。用户设的 `val` 只在"恰好没有 fade/auto_gain 生效"时才是最终值。这意味着：

- 若 Bus 增益直接乘进 `AudioSource::SetGain`，会被 `AudioPlayer` 的 `UpdateBuffer` 覆盖逻辑冲掉
- 必须在 **AudioSource 内部**维护一个独立的 `bus_gain` 因子，与实际源增益分离，最终 `AL_GAIN = own_gain × bus_gain`

---

## 二、设计目标

1. **Bus 树**：`Master → Music/SFX/Ambient/UI`（可扩展任意层级），每节点独立音量 + 静音
2. **增益正确合成**：`最终增益 = 源自身增益 × 所属 bus 链上所有节点增益`
3. **六条路径统一接入**，且不被各播放器的内部增益逻辑覆盖
4. **低频变更、即时生效**：调 bus 音量立刻反映到所有正在发声的源
5. **不破坏现有 API**：默认 bus 增益 1.0，现有代码行为不变

---

## 三、核心设计

### 3.1 AudioBus 类（新增）

```cpp
// inc/hgl/audio/AudioBus.h
namespace hgl::audio
{
    class AudioSource;   // 前向声明

    /**
     * 音频总线节点。构成一棵树：
     *   Master ─┬─ Music
     *           ├─ SFX
     *           ├─ Ambient
     *           └─ UI
     * 有效增益 = 本节点 gain × 父链所有 gain。
     */
    class AudioBus
    {
        AudioBus *parent;                     ///< 父总线（Master 为 nullptr）
        UnorderedSet<AudioBus *> children;    ///< 子总线
        UnorderedSet<AudioSource *> sources;  ///< 直接挂在此总线上的音源（用于增益变更推送）

        float gain;                           ///< 本节点增益（0.0-∞，默认 1.0）
        bool  mute;                           ///< 静音

        float cached_effective_gain;          ///< 缓存的有效增益（含父链）

        void RecalculateEffectiveGain();      ///< 自顶向下重算缓存，并推送所有源

    public:
        explicit AudioBus(AudioBus *parent=nullptr);
        ~AudioBus();

        AudioBus *GetParent()const{return parent;}

        void    SetGain(float);               ///< 设置本节点增益（0.0=静音，1.0=满）
        float   GetGain()const{return gain;}
        void    SetMute(bool);                ///< 静音
        bool    IsMute()const{return mute;}

        float   GetEffectiveGain()const{return mute?0.0f:cached_effective_gain;}

        AudioBus *CreateChild(const char *name);   ///< 创建子总线

        // 音源挂接（由 AudioSource 调用）
        void    AttachSource(AudioSource *);
        void    DetachSource(AudioSource *);
    };
}//namespace hgl::audio
```

**关键点**：
- `sources` 集合维护"直接挂在本 bus 上的源"，当 `SetGain`/`SetMute` 时**推送刷新**这些源（以及递归子 bus 的源）
- `cached_effective_gain` 缓存整条链的乘积，避免每次查询都向上遍历
- 用 `UnorderedSet`（CMCore 已有，SpatialAudioWorld 在用的容器）

### 3.2 AudioSource 改动（唯一注入点）

```cpp
// AudioSource.h 新增成员与接口
private:
    AudioBus *bus;          ///< 所属总线（nullptr = 未挂载，等同 Master 满增益）
    float     bus_gain;     ///< 缓存的总线有效增益（默认 1.0）

public:
    void    SetBus(AudioBus *);      ///< 挂载到总线（自动 Attach/Detach）
    AudioBus *GetBus()const{return bus;}

// AudioSource.cpp 核心改动
void AudioSource::SetGain(float _gain)
{
    if(!alSourcef)return;
    if(index==InvalidIndex)return;

    gain=_gain;
    ApplyGain();            // 抽取统一出口
}

void AudioSource::ApplyGain()        // 新增：唯一实际写 OpenAL 增益的地方
{
    const float final_gain = gain * bus_gain;
    alSourcef(index, AL_GAIN, final_gain);
}

void AudioSource::SetBus(AudioBus *b)
{
    if(bus) bus->DetachSource(this);
    bus=b;
    bus_gain = bus ? bus->GetEffectiveGain() : 1.0f;
    if(bus) bus->AttachSource(this);
    ApplyGain();
}

// 由 AudioBus 调用：总线增益变更后刷新
void AudioSource::OnBusGainChanged(float effective_gain)
{
    bus_gain = effective_gain;
    ApplyGain();
}
```

**设计要点**：
- `gain`（源自身）与 `bus_gain`（总线）**分离**，`ApplyGain()` 是唯一写 `alSourcef` 的地方
- `AudioPlayer` 的 `UpdateBuffer` 里 `SetGain(factor*gain)` 仍然只影响 `gain`，bus 增益独立保留，**不会被覆盖**
- 对现有 API 完全透明：未挂 bus 时 `bus_gain=1.0f`，行为不变

### 3.3 AudioBus 实现（增益推送）

```cpp
void AudioBus::SetGain(float g)
{
    gain = (g<0.0f)?0.0f:g;
    RecalculateEffectiveGain();
}

void AudioBus::SetMute(bool m)
{
    mute = m;
    RecalculateEffectiveGain();
}

void AudioBus::RecalculateEffectiveGain()
{
    // 自顶向下重算（本节点会传播给所有后代）
    float eff = gain;
    AudioBus *p = parent;
    while(p){ eff *= p->gain; p = p->parent; }

    cached_effective_gain = mute ? 0.0f : eff;

    // 推送本节点直接挂载的源
    for(AudioSource *s : sources)
        s->OnBusGainChanged(GetEffectiveGain());

    // 递归子总线（子总线的缓存也失效了）
    for(AudioBus *child : children)
        child->RecalculateEffectiveGain();
}
```

**复杂度**：`SetGain` 是 O(子树源数)，但这是**低频操作**（玩家在设置菜单拖音量条），游戏中可接受。若未来需要每帧渐变，可优化为"标记脏 + 播放线程惰性重算"。

---

## 四、六条路径接入方案

### 4.1 AudioPlayer / MIDIPlayer

**零改动即可接入**——因为它们最终都走 `audiosource.SetGain(...)`，而 bus 增益已在 AudioSource 内部独立维护。

```cpp
// 用户侧接入
AudioPlayer bgm;
bgm.Load("bgm.ogg");
bgm.audiosource... // 不暴露。需给 AudioPlayer 加一个接口：
bgm.SetBus(music_bus);   // 新增一行转发
```

需要给 `AudioPlayer`/`MIDIPlayer` 各加一个转发接口：

```cpp
// AudioPlayer.h
void SetBus(AudioBus *b){ audiosource.SetBus(b); }
```

验证点：`AudioPlayer::UpdateBuffer` 的 `audiosource.SetGain(factor*gain)` 与 `SetBus` 互不干扰——bus 增益在 `ApplyGain` 里独立相乘。

### 4.2 MIDIOrchestraPlayer（16 通道）

**16 个 `AudioSource` 全部挂到同一 bus**（如 Music）：

```cpp
// MIDIOrchestraPlayer.h
void SetBus(AudioBus *b)
{
    for(int i=0;i<MAX_MIDI_CHANNELS;i++)
        if(sources[i]) sources[i]->SetBus(b);
}
```

**注意**：通道自身增益 `channel_positions[ch].gain` 与 bus 增益是**相乘关系**（正确）。bus 静音会一次性静音整个交响乐团。

### 4.3 AudioManager

`AudioManager` 管理一个音源池。给它加一个 bus，池中所有源统一挂载：

```cpp
// AudioManager.h
void SetBus(AudioBus *b)
{
    for(int i=0;i<Items.GetCount();i++)
        Items[i]->source->SetBus(b);
}
```

**注意**：`AudioItem::Check()`（`AudioManager.cpp:32`）会 `delete source; source=new AudioSource;` 重建源——**重建后要重新 SetBus**，否则新源丢失总线挂载。这是接入时要处理的坑。

### 4.4 SpatialAudioWorld

`SpatialAudioSource::source` 是空间音频的内部源，距离衰减由 OpenAL 在 `AL_GAIN` 上**自动叠加**（OpenAL 内部：`final = AL_GAIN × distance_attenuation`）。

**Bus 增益乘进 AL_GAIN 后，与距离衰减是相乘关系，语义正确**——调低 SFX bus 音量，远处和近处的音源都等比降低。

```cpp
// SpatialAudioWorld 增加一个 bus 成员，Create() 时挂载
void SpatialAudioWorld::SetBus(AudioBus *b)
{
    scene_mutex.Lock();
    // 对已有源挂载；之后 Create() 的新源也挂
    world_bus = b;
    for(SpatialAudioSource *asi : source_list)
        if(asi->source) asi->source->SetBus(b);
    scene_mutex.Unlock();
}
```

**坑**：`Create()` 时 `asi->source` 可能尚未创建（惰性创建），需在 `ToHear`/实际创建 source 处补挂 bus。

### 4.5 直接使用 AudioSource 的用户

最直接：`src->SetBus(sfx_bus);`

---

## 五、增益链完整语义

接入后，一个音源的最终 OpenAL 增益为：

```
AL_GAIN = own_gain                              ← AudioSource::gain
        × bus_gain                               ← 所属 bus 链有效增益（新）
        × (fade/auto_gain 因子)                  ← AudioPlayer/MIDIPlayer 计算后并入 own_gain
        × (distance_attenuation)                 ← OpenAL 自动（空间音频）
        × (cone_gain / filter 等)                ← OpenAL 自动
```

**分层清晰**：Bus 只负责"类别音量"，源只负责"自身音量"，OpenAL 负责"空间/物理衰减"。

---

## 六、根总线（Master）的归属问题

Bus 树需要一个根。有两种方案：

| 方案 | 归属 | 优点 | 缺点 |
|---|---|---|---|
| A | 新增 `AudioEngine` 单例持有 `master_bus` | 集中、符合未来引擎化方向 | 又引入单例 |
| B | `AudioManager` 持有根，其他类引用 | 复用现有管理类 | AudioManager 目前是"音效池"语义，职责不符 |

**建议 A**：即便现在只做一个最小 `AudioEngine`（仅持有一个 `AudioBus *master` 并预建 Music/SFX/Ambient/UI 四子总线），也是为任务 4（update 驱动）预留的正确骨架。避免 Bus 树"悬空无根"。

```cpp
// 最小 AudioEngine（本报告范围只做 bus 部分）
class AudioEngine
{
    AudioBus master;
    AudioBus *music, *sfx, *ambient, *ui;
public:
    AudioEngine() {
        music   = master.CreateChild("Music");
        sfx     = master.CreateChild("SFX");
        ambient = master.CreateChild("Ambient");
        ui      = master.CreateChild("UI");
    }
    AudioBus *GetMaster() { return &master; }
    AudioBus *GetMusic()  { return music; }
    // ...
};
```

---

## 七、实现步骤（bite-sized）

1. **新增 `AudioBus.h/.cpp`**（约 150 行）：树结构 + gain/mute + 源集合 + 推送刷新
2. **改 `AudioSource`**（约 30 行）：加 `bus`/`bus_gain` 成员、`SetBus`/`OnBusGainChanged`、抽取 `ApplyGain()`（把现有 `SetGain`/`SetConeGain` 等所有写 AL_GAIN 的地方收敛到 `ApplyGain`）
3. **加转发接口**：`AudioPlayer::SetBus`、`MIDIPlayer::SetBus`、`MIDIOrchestraPlayer::SetBus`、`AudioManager::SetBus`、`SpatialAudioWorld::SetBus`（各 3-10 行）
4. **处理源重建坑**：`AudioManager::AudioItem::Check()` 重建 source 后重挂 bus
5. **新增最小 `AudioEngine`**（约 40 行）：master + 四子总线
6. **验证**：编译 + 新增一个 bus 测试 example（挂 SFX bus，调低后音效变轻，静音后无声）

---

## 八、验证方案

| 检查 | 方法 | 预期 |
|---|---|---|
| 编译 | 构建 CMAudio + examples | 0 error |
| 增益正确性 | 单源挂 SFX bus，`bus.SetGain(0.5)` | 输出幅度减半（可用 AudioMixer 或直接读 AL_GAIN 验证） |
| 静音 | `bus.SetMute(true)` | 无声 |
| 不被覆盖 | AudioPlayer 播放中设 fade，同时挂 bus | fade 与 bus 增益独立相乘 |
| 多源统一 | MIDIOrchestraPlayer 挂 Music bus 调低 | 16 通道同时降低 |
| 回归 | 现有 4 个 examples 全跑 | 输出与整改前一致（默认 bus=1.0） |

---

## 九、风险与权衡

1. **`AudioPlayer` 双写增益**：本方案通过"bus_gain 与 own_gain 分离 + ApplyGain 统一出口"根治，但需确认 `AudioPlayer::UpdateBuffer` 里**所有**写 gain 的地方都走 `SetGain`（当前是，未来改动需警惕）。
2. **推送刷新成本**：`SetGain` 递归 O(子树源数)。低频操作可接受；若未来每帧渐变，改为"脏标记 + 惰性重算"。
3. **SpatialAudioWorld 惰性建源**：需在所有实际 `source->Create()` 后补 `SetBus(world_bus)`。
4. **线程安全**：bus 增益是简单 float，`SetGain`/`ApplyGain` 的竞态可用 AudioPlayer 已有的 `lock` 或接受"一帧内生效"（游戏惯例）。若需严格，`bus_gain` 用 `atom<float>`。
5. **AudioManager 源池重建**：`Check()` 重建后必须重挂 bus，否则静音失效。

---

## 十、结构合理性总结

| 类 | 现有增益结构评价 | Bus 接入改动量 |
|---|---|---|
| `AudioSource` | ✅ 干净，是唯一汇聚点 | 核心改动（30 行） |
| `AudioPlayer` | ⚠️ 双写（SetGain vs UpdateBuffer），需 bus_gain 分离 | 转发接口 1 行 |
| `MIDIPlayer` | ⚠️ 同上 | 转发接口 1 行 |
| `MIDIOrchestraPlayer` | ⚠️ 16 通道 × auto_gain 两级 | 循环挂载 5 行 |
| `AudioManager` | ⚠️ 源池重建丢状态 | 转发 + 重建重挂 5 行 |
| `SpatialAudioWorld` | ⚠️ 距离衰减自动叠加，需在惰性建源处补挂 | 5-10 行 |

**结论**：六条路径虽然增益语义各异，但**全部收敛到 `AudioSource::SetGain`**，使 Bus 树只需在 `AudioSource` 一个类做核心改造（bus_gain 分离 + ApplyGain 出口），其余类只加转发接口。这是当前结构最有利的地方——也是建议**先做 Bus 树再做效果链**的原因（效果链同样挂在 Bus 节点上，Bus 是它的前置挂载点）。

*报告基于 CMAudio 全模块代码调研生成。*
