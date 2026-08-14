---
title: "插件系统"
linkTitle: "插件系统"
weight: 130
date: 2026-08-15
description: "解码插件（WAV/Vorbis/Opus）与 MIDI 合成器插件（6 种）"
draft: false
---

# 插件系统

CMAudio 的插件位于 `Plug-Ins/`，通过统一的插件接口（`AudioPlugInInterface`，
`GetInterface(version, ...)` 版本协商）挂接，编译为动态库（`add_cm_plugin`，始终 SHARED）。

## 插件清单

| 插件目录 | 类型 | 说明 |
|---|---|---|
| `Audio.Wav` | 解码 | RIFF/WAVE PCM |
| `Audio.Vorbis` | 解码 | OGG Vorbis（libogg + libvorbis） |
| `Audio.Opus` | 解码 | OGG Opus |
| `Audio.FluidSynth` | MIDI 合成 | SoundFont 合成（FluidLite） |
| `Audio.Timidity` | MIDI 合成 | GUS patch 合成 |
| `Audio.TinySoundFont` | MIDI 合成 | 轻量 SoundFont 合成 |
| `Audio.WildMIDI` | MIDI 合成 | WildMidi 合成 |
| `Audio.ADLMIDI` | MIDI 合成 | OPL3 FM 合成 |
| `Audio.OPNMIDI` | MIDI 合成 | OPN2 FM 合成 |

共 **9 个插件**：3 个解码器 + 6 个 MIDI 合成器。

## 加载与使用

插件在运行时按需加载（动态库），由 `AudioBuffer`/`AudioPlayer`/`MIDIPlayer` 等高层类自动选择：

- **解码器**：根据 `AudioFileType`（Wav/Vorbis/Opus）加载对应解码插件，把压缩音频解码成 PCM。
- **MIDI 合成器**：`MIDIPlayer`/`MIDIOrchestraPlayer` 通过合成器插件把 MIDI 事件渲染成 PCM。

> 使用方一般无需直接接触插件接口；扩展新格式时实现 `AudioPlugInInterface` 并注册到 `Plug-Ins/CMakeLists.txt`。

## 第三方依赖

- `Plug-Ins/libogg/`：Vorbis / Opus 共用的 OGG 容器解析。
- 各合成器插件内部自带的第三方实现（FluidLite、Timidity、TinySoundFont、WildMidi、ADLMIDI、OPNMIDI 等）。

> 这些 vendored 第三方源码树在统计代码量时应排除，避免误导。
