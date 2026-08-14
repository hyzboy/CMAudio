# CMAudio 重构复盘与引擎化补全计划

> 复盘范围：2026-08-14 以来 CMAudio 模块（`D:\ULRE\CMAudio`，ULRE 子模块）的系列重构
> 目的：对齐"已完成 vs 待做"，把剩余引擎化工作补全为可执行路线图
> 关联文档：`GameAudioModule_Analysis.md`（差距分析）、`AudioBus_Design_Analysis.md`（Bus 设计）、
> `AllClasses_Structure_Analysis.md`（全类结构）、`Naming_Audit_And_Rename_Plan.md`（命名方案）、
> `.hermes/plans/2026-08-14_143000-cmaudio-arch-refactor.md`（架构整改）

---

## 一、复盘：已完成的重构（4 条主线）

### 主线 1：架构整改 —— 消除 4 类系统性缺陷

| 任务 | 问题 | 改动 | 状态 |
|---|---|---|---|
| 任务 1 | volatile 伪同步 | `loop`/`ps`/`total_time` → `hgl::atom`，锁外写点审查 | 完成 |
| 任务 2 | const 冗余 + 封装粗糙 | 顶层 const 清理；`SpatialAudioSource` 状态收 private；`GetGain` friend → 成员 | 完成 |
| 任务 3 | OpenAL 类型泄漏 | `AudioDataInfo` 删 `format`；`openal::ToOpenALFormat/FromOpenALFormat` 边界转换 | 完成 |
| 任务 4 | 命名空间污染 | 15 头 + src 迁入 `hgl::audio`；清理全局/类内 `using namespace` | 完成 |

### 主线 2：系统性问题修复 + 不合理设计整改（6 项）

1. `AudioBuffer::SetData` 成功路径未置 `loaded=true` 的 OpenAL buffer 资源泄漏 → 修复 + `IsLoaded()`
2. `AudioFilterType`/`AudioFilterConfig` 从 `AudioSource.h` 抽离 → 独立 `AudioFilter.h`
3. 移除 `friend class AudioPlayer`，9 处内联函数改 public getter
4. `AudioMemoryPool` 三方法（`Ensure`/`Preallocate`/`EnsureWithEstimate`）收敛为单一 `Ensure`
5. `Listener.h` → `AudioListener.h`（git mv + 4 处引用）
6. `AudioMixerSourceConfig::operator==` 简化为组合 `AudioDataInfo`/`AudioFilterConfig` 的 `==`

### 主线 3：命名审核与重命名（R1-R4）

| 批次 | 内容 | 状态 |
|---|---|---|
| R1 语义修复 | `ok→loaded`、`ps→play_state`、`Index/index/source→buffer_id/source_id`、`Time/Size/Freq→duration/data_size/sample_rate`、`rate/format→sample_rate/al_format`、`MidiChannelInfo→MIDIChannelInfo` | 完成 |
| R2 风格统一 | `ref_dist/max_dist→reference_distance/max_distance`、`angle→cone_angle`、`gainhf→gain_hf`、局部变量展开（`fis/aft/asi/rd`）、大写成员小写化 | 完成 |
| R3 语义精化 | `type→interpolation_type/filter_type`、`pause→paused`、`GetCurTime→GetPlaybackTime`、`decode→decoder`、`auto_gain→gain_ramp`、`buffer[3]→al_buffers[3]`、`SpatialAudioWorld` 成员 | 完成 |
| R4 跨模块风格 | `AudioDataInfo`/`MixingTrack`/`MixerConfig`/`AudioMixerSourceConfig` 的 26 个 camelCase 字段 → snake_case | 完成 |

### 主线 4：插件体系整改

1. `WildMidi` → `WildMIDI`（目录 / 目标名 `CMP.Audio.WildMIDI` / 运行时名 / 文档引用，统一 Midi/MIDI 大小写）
2. 修复 `MIDIPlayer` 硬编码 `"MIDI"` 死引用 → `"Timidity"`（原无 `CMP.Audio.MIDI.dll`，加载永远失败）
3. 删除 `Tremolo`/`Tremor`（Vorbis 定点解码变体，现不需要）
4. README 架构图插件层 + 插件表格修正（`Audio.WAV→Wav`、`Audio.Ogg→Vorbis`、`Audio.MIDI→MIDI×6`）
5. `OpenALEECPort.cpp` 名称同步（死代码，未启用）

**当前状态**：核心库 + 4 examples + 9 插件全部构建 0 error、冒烟通过；Plug-Ins 目录干净（Wav / Vorbis / Opus + 6 个 MIDI 合成器）。

---

## 二、遗留收尾项（计划已列但未落地）

| # | 项 | 结论 | 状态 |
|---|---|---|---|
| 1 | `AudioDecode.h:27` 尾注释 | 已改为 `};//struct AudioFloatPlugInInterface` | 完成 |
| 2 | `PlayState`/`MIDIPlayState`/`MIDIOrchestraState` 的 `Exit` 枚举值 | 调研确认是"线程退出信号"（`Stop()`/播放完/淡出到 0 时设置，主循环检测后 `alSourceStop`+退出循环），与 `Thread::WaitExit()` 同命名域；改名 `Stopped`/`Finished` 会与 `AL_STOPPED`/播放结束混淆 → **保留 `Exit`**，仅修正 `AudioPlayer.cpp:462` 错误注释 | 完成（保留 + 注释修正） |
| 3 | `OpenALEECPort.cpp` | 未编译的死代码，暂不动 | — |

---

## 三、引擎化补全计划（剩余待做）

> 依据 `GameAudioModule_Analysis.md`：CMAudio 后端/DSP 底子扎实（解码、空间化、混音、MIDI 均引擎级），
> 缺的是"游戏音频引擎骨架"——总线、资源管理、统一更新循环、事件层。

```
┌─ 内容层（新增）──────────────────────────────┐
│  SoundBank / SoundEvent（数据驱动配置）        │
├─ 引擎层（新增，核心）─────────────────────────┤
│  AudioEngine：update() 驱动                   │
│   ├─ Bus 树（Master/Music/SFX/Ambient/UI）    │
│   ├─ Ducking / 动态混音 / 虚拟音源调度         │
│   └─ 设备管理（实例化、热切换）                │
├─ 资源层（新增）──────────────────────────────┤
│  AudioAssetManager：缓存/引用计数/异步加载     │
├─ 对象层（已有，增强）─────────────────────────┤
│  AudioSource/Buffer/Listener/SpatialAudioWorld│
├─ 后端层（已有）──────────────────────────────┤
│  OpenAL 封装 / 解码插件 / MIDI 合成器          │
└─ 工具层（已有 + 扩展）────────────────────────┘
```

### P0（架构缺口最大，优先）

**P0-1：Bus 树 + 分组音量/静音**（✅ 已实现）

实现步骤（7 步）：
1. 新增 `AudioBus.h/.cpp`（树结构 + gain/mute + 源集合 + 增益推送）
2. 改 `AudioSource`：加 `bus`/`bus_gain` 成员、`SetBus`/`OnBusGainChanged`，抽取 `ApplyGain()` 为唯一写 `alSourcef` 出口
3. 六路径转发接口：`AudioPlayer`/`MIDIPlayer`/`MIDIOrchestraPlayer`/`AudioManager`/`SpatialAudioWorld` 各加 `SetBus()`
4. 处理源重建坑：`AudioManager::AudioItem::Check()` 重建 source 后重挂 bus
5. 新增最小 `AudioEngine`（`master` + Music/SFX/Ambient/UI 四子总线）
6. 验证：编译 + 新增 bus 测试 example（调低 SFX 音量、静音）

**P0-2：AudioAssetManager（资源缓存 + 异步加载）**（✅ 已实现）

实现要点：
- `AudioBuffer` 加 `atom<uint> ref_count` + `IncRef/DecRef/GetRefCount`
- `AudioAssetManager`：`Acquire/Release/Register/Find/Contains/Clear`（缓存去重 + 引用计数归零卸载）
- 解码/上传分离：`DecodeAudio`（纯 CPU，后台线程调用）+ `UploadDecoded`（主线程上传到 OpenAL buffer）
- 后台解码线程 `AudioLoadThread`（读文件 + 插件解码，不碰 OpenAL，原子 task_count 防轮询空窗）+ `AcquireAsync/Update/IsLoading/GetPendingCount`
- `SuggestAudioLoadMode(file_size)`：全量 vs 流式启发式决策（默认阈值 1MB）
- 测试：`asset_manager_test`（37 项纯逻辑）+ `async_load_test`（17 项端到端，OpenAL null 设备 + 真实 wav 解码）

### P1（引擎化 + 生产力）

**P1-1：`AudioEngine::update()` 统一驱动**
- 收拢 `SpatialAudioWorld` 每帧更新 + 音量淡变/位置插值的集中调度
- 音频线程缓冲，主线程卡顿（GC/场景加载）不断流

**P1-2：SoundEvent 数据驱动层**
- 事件名 → 配置（音量/音高随机化/衰减/优先级/循环/分组）
- 配 TOML/JSON（项目已有 TOML 解析先例）

### P2（体验质感）

- Ducking/侧链（音乐在语音/重要音效时自动压低）
- 动态音乐分层（战斗/探索状态切换 + crossfade）
- AudioCapture（录音；`alcCapture` 函数指针已就绪）

### P3（特定需求时再做）

- 音频分析（频谱/FFT/RMS/节拍检测）
- 响度归一化（EBU R128/LUFS）
- 移动平台策略（iOS 静音开关 / Android Audio Focus）

---

## 四、建议执行顺序

1. ~~遗留收尾~~（#1/#2 已完成，见上表；#3 暂不动）
2. ~~**P0-1 Bus 树**~~（✅ 已实现：`AudioBus.h/.cpp` + `AudioEngine.h` + 六路径 `SetBus` + `bus_tree_test` 全过）
3. ~~**P0-2 AudioAssetManager**~~（✅ 已实现：缓存去重 + 引用计数 + 后台解码线程 + 异步 API）
4. **P1-1 AudioEngine::update()**（在 Bus + 资源管理之上收拢）
5. **P1-2 SoundEvent**
6. **P2 / P3 按需**

**关键依赖**：效果链（Effect Chain）挂在 Bus 节点上 → Bus 树先于效果链；`AudioEngine::update()` 建立在 Bus 树 + 资源管理之上。

*本复盘基于 2026-08-14 以来全部重构记录与四份分析文档生成。*
