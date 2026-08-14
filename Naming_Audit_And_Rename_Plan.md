# CMAudio 命名审核与重命名方案

> 审核对象：CMAudio 全部头文件（inc/hgl/audio/*.h）与实现（src/*.cpp）
> 审核范围：类名、结构名、枚举名、枚举值、成员变量名、函数参数名、局部变量名
> 目标：分析每个名称的具体存在意义，识别语义模糊/误导、风格不一致、缩写混乱，
>       制定一套更准确科学合理的重命名方案。

---

## 〇、命名基准约定

审核基于以下约定（hgl 系列库的既有风格，也是本方案的目标风格）：

| 类别 | 约定 | 示例 |
|---|---|---|
| 类型名（类/结构/枚举） | PascalCase | `AudioSource`、`AudioDataInfo`、`PlayState` |
| 枚举值 | PascalCase | `None`、`Linear`、`Play` |
| 成员变量 | snake_case（小写+下划线） | `sample_rate`、`ref_distance` |
| 函数/方法 | PascalCase（动词开头） | `SetGain()`、`IsPlaying()` |
| 常量 | PascalCase | `MinPitch` |
| 局部变量/参数 | snake_case | `file_stream`、`target_gain` |

**核心矛盾**：CMAudio 存在**两套成员命名风格并存**——
- `AudioSource` / `AudioPlayer` / `SpatialAudio*` 用 **snake_case**（`ref_dist`、`cone_gain`、`rolloff_factor`）
- `AudioMixer` 系列（`MixingTrack` / `MixerConfig` / `AudioDataInfo` / `AudioMixerSourceConfig`）用 **camelCase**（`sourceIndex`、`masterVolume`、`bitsPerSample`）

这是最系统性的命名问题，需单独决策（见第五节）。

---

## 一、语义模糊 / 误导的命名（最高优先级，必改）

### 1.1 `ok`（AudioBuffer 成员）

```cpp
bool ok;   // AudioBuffer.h
```

- **实际意义**：缓冲区是否已成功加载/写入数据（`SetData` 成功时置 true，`Clear` 时置 false，析构时据此判断是否要 `alDeleteBuffers`）
- **问题**：`ok` 是意义最空洞的变量名之一，无法表达"加载成功"还是"操作成功"；且之前已发现它因 `SetData` 未置 true 导致资源泄漏 bug
- **建议**：`loaded`（是否已加载数据）
- **配套**：getter `IsValid()` → `IsLoaded()`（语义更精准）

### 1.2 `ps`（AudioPlayer 成员）

```cpp
atom<PlayState> ps;   // AudioPlayer.h
```

- **实际意义**：播放器当前状态机（None/Play/Pause/Exit）
- **问题**：`ps` 是毫无信息量的双字母缩写
- **建议**：`play_state`（配合 `GetPlayState()` 更自然）

### 1.3 `Index` / `index` 语义冲突（跨类）

| 类 | 当前名 | 实际意义 | 问题 |
|---|---|---|---|
| `AudioBuffer` | `uint Index` | OpenAL **buffer** 句柄 | 大写成员 + 语义不清 |
| `AudioSource` | `uint index` | OpenAL **source** 句柄 | 与 AudioBuffer 的 Index 同名异义 |
| `AudioPlayer` | `ALuint source` | OpenAL **source** 句柄（转发 audiosource） | 与 AudioSource::index 重复 |

- **问题**：三个类里 `index`/`Index`/`source` 指向不同的 OpenAL 对象，却用了混淆的名字；`AudioPlayer::source` 与 `AudioSource` 类型名还撞名
- **建议**：
  - `AudioBuffer::Index` → `buffer_id`
  - `AudioSource::index` → `source_id`
  - `AudioPlayer::source` → `source_id`（并区分于 `audiosource` 对象成员）

### 1.4 `Time` / `Size` / `Freq`（AudioBuffer 成员）

```cpp
double Time;    // 实际：可播放时长（秒）
uint   Size;    // 实际：数据总字节数
uint   Freq;    // 实际：采样率（Hz）
```

- **问题**：三个都是大写成员（违反 snake_case），且 `Time` 实际是 duration、`Freq` 实际是 sample rate，名字偏泛
- **建议**：`duration` / `data_size` / `sample_rate`

### 1.5 `rate` / `format`（AudioPlayer / MIDIPlayer / MIDIOrchestraPlayer 成员）

```cpp
ALenum  format;    // 实际：OpenAL 格式枚举
ALsizei rate;      // 实际：采样率
```

- **问题**：`rate` 语义模糊（采样率？比特率？），`format` 未表明是 AL 后端格式（且与纯 CPU 层 `AudioDataInfo` 的格式抽象并存易混）
- **建议**：`al_format` / `sample_rate`

### 1.6 `MidiChannelInfo`（结构名）

```cpp
struct MidiChannelInfo   // AudioDecode.h
```

- **实际意义**：MIDI 通道信息
- **问题**：缩写大小写不一致——同文件/同模块里都是全大写 `MIDI`（`MIDIPlayer`、`MIDIInstrument`、`MIDIOrchestraPlayer`、`MIDIPlayState`），唯独这里是 `Midi`
- **建议**：`MIDIChannelInfo`

---

## 二、缩写不一致 / 简称过短（高优先级）

### 2.1 同一概念命名分裂

| 概念 | AudioSource 用名 | SpatialAudioSource 用名 | 建议统一 |
|---|---|---|---|
| 参考距离 | `ref_dist` | `ref_distance` | `reference_distance` |
| 最大距离 | `max_dist` | `max_distance` | `max_distance` |
| 锥形角度 | `angle`（ConeAngle 类型） | `cone_angle` | `cone_angle` |

- **问题**：同一个"参考距离"概念在两个类里写法不同，`ref_dist`/`max_dist` 还带下划线缩写
- **建议**：统一为 `reference_distance` / `max_distance` / `cone_angle`

### 2.2 `gainhf` 缩写不一致

| 位置 | 当前名 | 建议 |
|---|---|---|
| `AudioFilterConfig` | `gain_hf` | ✓ 保留（正确） |
| `SpatialAudioSource` | `last_filter_gainhf` | `last_filter_gain_hf` |
| `SpatialAudioWorld` | `scene_lowpass_gainhf` | `scene_lowpass_gain_hf` |

- **问题**：`gainhf` 是 `gain_hf` 的退化写法，同一概念两种拼法
- **建议**：统一 `gain_hf`（`hf` = high-frequency，缩写本身可接受，但需加下划线）

### 2.3 局部变量/参数过短

| 位置 | 当前名 | 实际意义 | 建议 |
|---|---|---|---|
| `AudioPlayer::Load` / `MIDIPlayer::Load` | `fis` | 文件输入流 | `file_stream` |
| `AudioBuffer::Load` 参数 | `aft` | 音频文件类型 | `file_type`（或 `audio_file_type`） |
| `SpatialAudioWorld` 各处 | `asi` | SpatialAudioSource 指针 | `spatial_source` |
| `AudioMixerScene` | `rd` | std::random_device | `random_device` |
| `AudioPlayer::AutoGain` 参数 | `cur_time` | 当前时间 | ✓ 可接受（语义清晰） |

---

## 三、大写成员变量（违反 snake_case，中优先级）

| 类 | 当前名 | 建议 |
|---|---|---|
| `AudioManager` | `Items` | `items` |
| `AudioSource` | `Buffer`（AudioBuffer*） | `buffer` |
| `AudioBuffer` | `Index`/`Time`/`Size`/`Freq` | 见 1.3/1.4（`buffer_id`/`duration`/`data_size`/`sample_rate`） |

- **问题**：这些是早期代码遗留的大写开头成员，与同文件其他 snake_case 成员（`gain`、`loop`）冲突
- **建议**：全部小写化，并顺带语义化（见第一节）

---

## 四、语义泛化 / 命名不精确（中低优先级）

### 4.1 `type` 成员名过泛

| 位置 | 当前名 | 实际意义 | 建议 |
|---|---|---|---|
| `GainRamp` | `InterpolationType type` | 插值类型 | `interpolation_type` |
| `AudioFilterConfig` | `AudioFilterType type` | 滤波器类型 | `filter_type` |

- **问题**：`type` 太泛，且 `GainRamp::type` 与 `AudioFilterConfig::type` 是不同枚举类型，阅读时需回溯声明
- **建议**：分别改为 `interpolation_type` / `filter_type`

### 4.2 `pause` vs `paused`（AudioSource）

```cpp
bool pause;   // 是否暂停
```

- **问题**：成员是状态标志，用动词原形 `pause` 容易与 `Pause()` 方法混淆
- **建议**：`paused`（形容词，状态标志惯例）

### 4.3 `Buffer` 语义（AudioSource）

```cpp
AudioBuffer *Buffer;   // AudioSource.h
```

- **实际意义**：绑定的音频缓冲区
- **建议**：`buffer`（小写 + 语义即"绑定的 buffer"，与 `SpatialAudioSource::buffer` 一致）

### 4.4 `CurTime` / `cur_time` 语义

- `AudioSource::GetCurTime()/SetCurTime()` → 返回的是 OpenAL 的当前播放秒数
- `SpatialAudioSource::cur_time` / `SpatialAudioWorld::cur_time` → 场景/源的时间戳
- **问题**：三处 `cur_time` 语义不同（播放位置 vs 更新时间戳），同名易混
- **建议**：
  - `AudioSource::GetCurTime` → `GetPlaybackTime`（播放位置）
  - `SpatialAudioSource::cur_time` → `last_update_time`（更新时间戳）
  - `SpatialAudioWorld::cur_time` → `current_time`（场景当前时间）

---

## 五、跨模块风格分裂（系统性，需决策）

`AudioDataInfo` / `MixingTrack` / `MixerConfig` / `AudioMixerSourceConfig` 的成员用 **camelCase**，与模块其余部分的 **snake_case** 分裂：

| 结构 | 当前字段（camelCase） | snake_case 化 |
|---|---|---|
| `AudioDataInfo` | `sampleRate` `channels` `bitsPerSample` `isFloat` `dataSize` | `sample_rate` `channels` `bits_per_sample` `is_float` `data_size` |
| `MixingTrack` | `sourceIndex` `timeOffset` `volume` `pitch` | `source_index` `time_offset` `volume` `pitch` |
| `MixerConfig` | `masterVolume` `useSoftClipper` `useDither` `normalize` | `master_volume` `use_soft_clipper` `use_dither` `normalize` |
| `AudioMixerSourceConfig` | `minCount` `maxCount` `minInterval` ... `filterConfig` `filterRandom` | `min_count` `max_count` `min_interval` ... `filter_config` `filter_random` |

**影响评估**：
- `AudioDataInfo` 是任务 3 刚重构的核心类型，被 AudioMixer / AudioMixerScene / AudioResampler / AudioBuffer / OpenAL / 4 个 examples 广泛引用
- 重命名会波及约 15 个文件、上百处引用
- 但收益是**消除模块内两套命名风格并存的混乱**，且当前 ULRE 对 CMAudio 零调用、无外部兼容负担

**建议**：作为独立一步执行（工作量最大但一次性解决），与前面四节的局部重命名分开提交。

---

## 六、执行顺序建议

按"语义修复 → 风格统一 → 跨模块大改"分层，每层独立可验证（构建 + examples 回归）：

| 批次 | 内容 | 波及文件 | 风险 |
|---|---|---|---|
| **R1（语义修复）** | `ok→loaded`、`ps→play_state`、`Index/index/source→buffer_id/source_id`、`Time/Size/Freq→duration/data_size/sample_rate`、`rate/format→sample_rate/al_format`、`MidiChannelInfo→MIDIChannelInfo` | ~8 头 + ~8 实现 | 低（机械替换，语义清晰） |
| **R2（风格统一）** | `ref_dist/max_dist→reference_distance/max_distance`、`angle→cone_angle`、`gainhf→gain_hf`、局部变量 `fis/aft/asi/rd` 展开、大写成员小写化（`Items/Buffer`） | ~6 头 + ~6 实现 | 低 |
| **R3（语义精化）** | `type→interpolation_type/filter_type`、`pause→paused`、`CurTime→PlaybackTime` 等 | ~4 头 + ~4 实现 | 中（改 API 名，需同步调用点） |
| **R4（跨模块风格）** | `AudioDataInfo`/`MixingTrack`/`MixerConfig`/`AudioMixerSourceConfig` camelCase→snake_case | ~15 文件 | 中高（量大但机械） |

---

## 七、完整重命名对照表

### 成员变量

| 文件 | 当前名 | 建议名 | 批次 |
|---|---|---|---|
| AudioBuffer.h/.cpp | `ok` | `loaded` | R1 |
| AudioBuffer.h/.cpp | `Index` | `buffer_id` | R1 |
| AudioBuffer.h/.cpp | `Time` | `duration` | R1 |
| AudioBuffer.h/.cpp | `Size` | `data_size` | R1 |
| AudioBuffer.h/.cpp | `Freq` | `sample_rate` | R1 |
| AudioSource.h/.cpp | `index` | `source_id` | R1 |
| AudioSource.h/.cpp | `pause` | `paused` | R3 |
| AudioSource.h/.cpp | `ref_dist` | `reference_distance` | R2 |
| AudioSource.h/.cpp | `max_dist` | `max_distance` | R2 |
| AudioSource.h/.cpp | `angle` | `cone_angle` | R2 |
| AudioSource.h/.cpp | `Buffer` | `buffer` | R2 |
| AudioPlayer.h/.cpp | `ps` | `play_state` | R1 |
| AudioPlayer.h/.cpp | `source` | `source_id` | R1 |
| AudioPlayer.h/.cpp | `format` | `al_format` | R1 |
| AudioPlayer.h/.cpp | `rate` | `sample_rate` | R1 |
| AudioPlayer.h/.cpp | `buffer[3]` | `al_buffers[3]` | R3 |
| AudioPlayer.h/.cpp | `decode` | `decoder` | R3 |
| AudioPlayer.h/.cpp | `auto_gain` | `gain_ramp`（语义更准） | R3 |
| AudioPlayer / MIDIPlayer | `audio_data`/`audio_ptr`/`audio_buffer`... | 保留（语义可接受） | — |
| MIDIPlayer.h/.cpp | 同 AudioPlayer 的 `ps/format/rate/decode` | 同上 | R1/R3 |
| MIDIOrchestraPlayer | `play`/`pause_state`/`state` | 保留或 `state` 统一 | R3 |
| AudioManager.h/.cpp | `Items` | `items` | R2 |
| GainEnvelope.h | `GainRamp::type` | `interpolation_type` | R3 |
| SpatialAudioSource | `is_play` | `should_play` | R3 |
| SpatialAudioSource | `last_pos`/`cur_pos` | `last_position`/`current_position` | R2 |
| SpatialAudioSource | `last_time`/`cur_time` | `last_update_time` | R3 |
| SpatialAudioSource | `move_speed` | `movement_speed` | R2 |
| SpatialAudioSource | `last_filter_gainhf` | `last_filter_gain_hf` | R2 |
| SpatialAudioWorld | `scene_lowpass_gainhf` | `scene_lowpass_gain_hf` | R2 |
| SpatialAudioWorld | `cur_time` | `current_time` | R3 |
| AudioFilter.h | `type` | `filter_type` | R3 |

### 结构名 / 枚举名

| 文件 | 当前名 | 建议名 | 批次 |
|---|---|---|---|
| AudioDecode.h | `MidiChannelInfo` | `MIDIChannelInfo` | R1 |
| AudioDecode.h | `AudioFloatPlugInInterface` 的尾注释 `//struct AudioPlugInInterface`（错误） | `AudioFloatPlugInInterface` | R1 |

### 方法名

| 类 | 当前名 | 建议名 | 批次 |
|---|---|---|---|
| AudioSource | `GetCurTime/SetCurTime` | `GetPlaybackTime/SetPlaybackTime` | R3 |
| AudioBuffer | `IsValid()` | `IsLoaded()` | R1 |

### 局部变量/参数

| 位置 | 当前名 | 建议名 | 批次 |
|---|---|---|---|
| AudioPlayer/MIDIPlayer::Load | `fis` | `file_stream` | R2 |
| AudioBuffer::Load | `aft` | `file_type` | R2 |
| SpatialAudioWorld | `asi` | `spatial_source` | R2 |
| AudioMixerScene | `rd` | `random_device` | R2 |
| AudioMixerScene | `rng` | 保留 | — |

---

## 八、其他发现的命名相关瑕疵

1. **`AudioFloatPlugInInterface` 的尾注释错误**（AudioDecode.h:27）：`};//struct AudioPlugInInterface` 实际应是 `AudioFloatPlugInInterface`（复制粘贴遗留）
2. **`AudioManager.h` 尾注释**：`}//namespace hgl::audio::audio` 多了一个 `::audio`（应为 `hgl::audio`）
3. **`GainRamp` 类名与用途**：类名是通用的"增益斜坡"，但它在 AudioPlayer/MIDIPlayer 里专门用作"自动增益过渡"，成员名 `auto_gain` 更贴切——两者语义有轻微错位（见 R3 的 `auto_gain→gain_ramp` 或反之，需二选一统一）
4. **`Exit` 枚举值**（PlayState/MIDIPlayState）：语义是"退出/停止"，但实际用作"播放结束"状态，与 `Stop()` 方法语义重叠，可考虑改名 `Stopped` 或 `Finished`（需确认线程退出语义后再定）

---

## 九、结论

CMAudio 的命名问题集中在三类：
1. **语义模糊**（`ok`/`ps`/`Index`/`Time`/`Freq`）—— 影响可读性与正确性（`ok` 已引发过 bug）
2. **风格不一致**（camelCase 与 snake_case 并存、缩写不统一）—— 影响维护性
3. **语义冲突**（多处 `index`/`cur_time`/`type` 同名异义）—— 影响理解

建议按 R1→R4 四批次执行，每批独立构建 + examples 回归验证。R4（AudioDataInfo 等 camelCase→snake_case）工作量最大，可单独决策是否执行。

*本报告基于 CMAudio 全部头文件与实现文件的逐一命名采集生成。*
