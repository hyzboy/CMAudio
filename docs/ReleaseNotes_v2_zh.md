# CMAudio v2 发行说明（从 CMAudioOLD 升级）

## 概述

本次版本更新以新增能力为主，而非大规模 API 变更。旧版本在 API 细节上几乎没有系统性的更新，本次版本主要新增混音器、空间音频以及更完整的 MIDI 播放能力，同时对少量类名做了词法/语法层面的修正。

## 亮点

- 新增 AudioMixer，支持离线/循环混音工作流。
- 增强空间音频管线与相关工具。
- 更完整的 MIDI 播放与多后端插件支持。
- 管理类命名更统一、清晰。

## 新模块与功能

### 音频混音器

- 新增 `AudioMixer` 与 `AudioMixerTypes`。
- 支持每轨时间偏移、音量、音调控制。
- 通过 `MixerConfig` 进行混音器配置。
- 参考文档：[CMAudio/AudioMixer_Usage.md](../AudioMixer_Usage.md)。

### 空间音频

- 新增 `SpatialAudioWorld` 空间音频场景系统。
- 新增方向性增益模型 `DirectionalGainPattern`。
- 新增混响预设 `ReverbPreset`。
- 新增插值类型 `InterpolationType`。

### MIDI 播放

- 新增 MIDI 播放栈：`MIDIPlayer`、`MIDIInstrument`、`MIDIOrchestraPlayer`。
- 通过插件支持多种 MIDI 后端（详见下文）。

### 内存与管理工具

- 新增 `AudioMemoryPool` 进行缓冲区内存管理。
- 管理类统一为 `AudioManager`。

## API 命名修正（词法/语法层面）

- `AudioManage` -> `AudioManager`
  - 头文件由 `AudioManage.h` 替换为 `AudioManager.h`。
  - 实现文件由 `AudioManage.cpp` 替换为 `AudioManager.cpp`。

> 说明：本次并未进行大规模 API 重构，主要为新增功能与少量命名修正。

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

- 使用 `AudioManage` 的项目请改用 `AudioManager`，包括 include 与类名。
- 现有播放、缓冲、场景等 API 保持可用；新增功能为增量式增强。

## 建议后续动作

- 阅读并集成混音器使用文档。
- 根据平台和授权选择合适的 MIDI 后端插件。
- 引入空间音频相关工具以增强方向性与环境效果。
