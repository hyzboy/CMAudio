---
title: "CMAudio 技术手册"
linkTitle: "技术手册"
weight: 1
date: 2026-08-15
description: "CMAudio 音频子系统各模块的技术手册与使用指南"
draft: false
---

# CMAudio 技术手册

本目录是 CMAudio 各模块的技术手册，按 [Hugo](https://gohugo.io) 站点格式编写
（YAML front matter + Markdown 正文），可直接作为 Hugo 站点 content 目录使用。

## 模块总览

CMAudio 的模块按功能域分为以下层次：

```text
┌─────────────────────────────────────────────────────────────┐
│  上层：数据驱动与场景                                        │
│  SoundEventManager · DynamicMusic · AudioMixerScene        │
├─────────────────────────────────────────────────────────────┤
│  中层：空间音频与播放                                        │
│  SpatialAudioWorld · AudioPlayer · AudioSource · AudioManager│
├─────────────────────────────────────────────────────────────┤
│  基础：总线 · 资源 · 录音                                    │
│  AudioEngine · AudioBus · AudioAssetManager · AudioCapture  │
├─────────────────────────────────────────────────────────────┤
│  DSP 工具链（纯 CPU，不依赖 OpenAL EFX）                     │
│  BiquadFilter · ParametricEQ · Compressor · LoudnessNormalizer│
│  TimeEffects · AudioAnalysis · AudioResampler · AudioMixer  │
├─────────────────────────────────────────────────────────────┤
│  后端：OpenAL Soft 绑定 + 插件 + 会话策略                    │
│  OpenAL · Plug-Ins(解码/MIDI) · AudioSessionPolicy          │
└─────────────────────────────────────────────────────────────┘
```

## 手册目录

| 手册 | 说明 |
|---|---|
| [快速开始](getting-started.md) | 环境准备、构建、最小播放示例 |
| [核心引擎](core-engine.md) | AudioEngine / AudioManager / AudioPlayer / AudioSource / AudioListener |
| [音频总线](audio-bus.md) | AudioBus 树、增益、Ducking、侧链压缩 |
| [资源管理](asset-management.md) | AudioAssetManager / AudioBuffer / 异步加载 |
| [声音事件与动态音乐](sound-events.md) | SoundEventManager / DynamicMusic |
| [录音](audio-capture.md) | AudioCapture |
| [空间音频](spatial-audio.md) | SpatialAudioWorld / 方向性 / 混响 / 频率衰减 |
| [DSP 滤波](dsp-filters.md) | BiquadFilter / ParametricEQ / AudioEQ |
| [DSP 动态与响度](dsp-dynamics.md) | Compressor / LoudnessNormalizer / 侧链 |
| [DSP 时域效果](dsp-time-effects.md) | DelayLine / Echo / Chorus / Flanger |
| [频谱分析](dsp-analysis.md) | FFT / RMS / 频谱 / onset |
| [重采样与混音](resample-mix.md) | AudioResampler / AudioMixer / AudioMixerScene |
| [MIDI 播放](midi.md) | MIDIPlayer / MIDIOrchestraPlayer |
| [插件系统](plugins.md) | 解码插件 / MIDI 合成器插件 |
| [会话策略](session-policy.md) | AudioSessionPolicy（移动平台） |

## 命名与类型约定

- 命名空间统一为 `hgl::audio`。
- `uint` / `os_char` / `OSString` / `OS_TEXT()` 等基础类型来自 CMCore（`hgl/CoreType.h`）。
- PCM 格式用 `AudioDataInfo{sample_rate, channels, bits_per_sample, is_float, data_size}` 描述，
  不依赖 OpenAL 的 `AL_FORMAT_*` 枚举（后端格式只在 OpenAL 边界转换）。
- 源码使用 UTF-8 BOM + CRLF 行尾；本文档（doc/）为 LF 无 BOM。
