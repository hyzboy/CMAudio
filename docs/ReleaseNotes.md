# CMAudio 发行说明

## 概述

本次版本以新增能力为主，兼顾少量命名修正，不涉及大规模 API 重构。重点补齐混音器、空间音频与 MIDI 播放能力，并完善相关工具类型，使模块划分更清晰、职责更集中。

## 亮点

- 引入离线/循环混音流程，面向批处理与制作型工作流。
- 空间音频场景化能力完整化，方向性与混响控制更细致。
- MIDI 播放栈更完整，后端插件选择更丰富。
- 管理类命名统一，降低迁移成本。

## 模块识别与能力划分

### 音频混音器

- 新增 `AudioMixer` 与 `AudioMixerTypes`，提供混音器核心与基础类型定义。
  - `AudioMixer` 作为离线混音入口，组织混音流程、轨道管理与输出控制。
  - `AudioMixerTypes` 提供混音所需的枚举与结构体，保持与运行时音频播放逻辑解耦。
- 增加混音场景与轨道配置能力：`AudioMixerScene`、`AudioMixerSourceConfig`。
  - `AudioMixerScene` 用于集中管理多轨场景，统一调度开始时间与整体渲染顺序。
  - `AudioMixerSourceConfig` 描述单轨资源、时间偏移、循环策略与混音参数。
- 每轨支持时间偏移、音量与音调控制，适配离线拼接与循环混音。
- 通过 `MixerConfig` 进行混音器参数组织与序列化管理。
  - 支持批处理配置复用，便于在制作/导出场景中保持一致性。

### 空间音频

- 新增 `SpatialAudioWorld`，提供空间音频场景与对象管理入口。
  - 负责空间对象生命周期、空间参数更新与全局空间配置。
  - 可作为场景级入口统一应用混响与方向性策略。
- 新增方向性增益模型 `DirectionalGainPattern`，用于描述声源指向性分布。
  - 适配不同指向性曲线，便于在声音定位与指向性设计中快速切换模型。
- 新增混响预设 `ReverbPreset`，用于快速构建环境混响参数。
  - 提供常见空间声学环境的参数模板，减少参数调试成本。
- 新增插值类型 `InterpolationType`，用于空间与参数过渡的插值策略选择。
  - 用于移动、旋转与参数变更的平滑过渡，降低跳变带来的听感突兀。
- 与现有 `Listener`、`ConeAngle` 等类型配合，形成完整空间音频管线。

### MIDI 播放

- 新增 MIDI 播放栈：`MIDIPlayer`、`MIDIInstrument`、`MIDIOrchestraPlayer`。
  - `MIDIPlayer` 作为播放控制入口，负责加载、播放与停止等基础控制。
  - `MIDIInstrument` 管理乐器/音色资源与参数，面向音色映射与配置。
  - `MIDIOrchestraPlayer` 面向多乐器编排场景，统一管理多路 MIDI 输出。
- 通过插件体系支持多种 MIDI 后端（详见插件更新部分）。
- 统一 MIDI 播放控制入口，便于切换合成器实现与资源管理策略。

### 内存与管理工具

- 新增 `AudioMemoryPool`，用于缓冲区的内存池管理与复用。
  - 提升频繁分配/释放场景的效率与稳定性，减少内存碎片。
- 管理类统一为 `AudioManager`，职责更集中、命名更明确。
  - 统一音频系统初始化、资源与设备的生命周期管理入口。

## API 命名修正（词法/语法层面）

- `AudioManage` -> `AudioManager`
  - 头文件由 `AudioManage.h` 替换为 `AudioManager.h`。
  - 实现文件由 `AudioManage.cpp` 替换为 `AudioManager.cpp`。

> 说明：本次不涉及大规模 API 重构，主要为新增功能与少量命名修正。

## 插件更新

### 新增插件

- `Audio.ADLMIDI`
- `Audio.FluidSynth`
- `Audio.OPNMIDI`
- `Audio.Timidity`
- `Audio.TinySoundFont`
- `Audio.WildMidi`

### 仍然支持的插件

- `Audio.Opus`
- `Audio.Tremolo`
- `Audio.Tremor`
- `Audio.Vorbis`
- `Audio.Wav`
- `libogg`

## 新增头文件（公开 API）

- `AudioMixer.h`
- `AudioMixerTypes.h`
- `AudioManager.h`
- `AudioMemoryPool.h`
- `DirectionalGainPattern.h`
- `InterpolationType.h`
- `MIDIInstrument.h`
- `MIDIOrchestraPlayer.h`
- `MIDIPlayer.h`
- `ReverbPreset.h`
- `SpatialAudioWorld.h`

## 新增实现文件

- `AudioMixer.cpp`
- `AudioManager.cpp`
- `DirectionalGainPattern.cpp`
- `MIDIInstrument.cpp`
- `MIDIOrchestraPlayer.cpp`
- `MIDIPlayer.cpp`
- `ReverbPreset.cpp`
- `SpatialAudioWorld.cpp`

## 迁移说明

- 使用 `AudioManage` 的项目请改用 `AudioManager`，包含 include 与类名替换。
- 现有播放、缓冲、场景等 API 保持可用；新增能力为增量式增强。

## 建议后续动作

- 根据工程需求规划混音场景与轨道配置策略。
- 结合平台授权与部署体量选择合适的 MIDI 后端插件。
- 在空间音频项目中引入方向性与混响预设以提升沉浸感。
