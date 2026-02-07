# MIDIOrchestraPlayer 技术手册

## 概述

`MIDIOrchestraPlayer` 是面向“多通道 3D 空间演奏”的 MIDI 播放器。它将每个 MIDI 通道映射为独立的 `AudioSource` 与 3D 位置，支持预设乐队布局与实时位置调整，可用于沉浸式音乐或虚拟乐团场景。

## 主要特性

- 16 个 MIDI 通道独立 `AudioSource`。
- 预设布局：交响乐/室内乐/爵士/摇滚。
- 通道位置/音量/启用状态可独立控制。
- 多通道同步播放。
- 支持 SoundFont 与采样率设置。

## 关键能力

### 播放控制

- `Load()` / `Load(io::InputStream*)`：加载 MIDI。
- `Play()` / `Pause()` / `Resume()` / `Stop()` / `Close()`：播放控制。

### 布局与空间控制

- `SetLayout(layout)`：切换预设布局。
- `SetOrchestraCenter(center)`：设置乐队中心。
- `SetOrchestraScale(scale)`：设置乐队缩放。
- `SetChannelPosition(channel, pos)`：单通道位置设置。
- `SetChannelVolume(channel, volume)`：通道音量。
- `SetChannelEnabled(channel, enabled)`：通道启用状态。
- `GetChannelSource(channel)`：访问通道 `AudioSource`。

### MIDI 配置

- `SetSoundFont(path)`：设置音色库。
- `SetBank(bank_id)`：设置音色库ID。
- `SetSampleRate(rate)`：设置采样率。

## 使用流程

1. 创建 `MIDIOrchestraPlayer`。
2. 设置 SoundFont 与采样率。
3. `Load()` MIDI。
4. `SetLayout()` 选择布局。
5. `Play()` 开始播放。

## 示例代码

```cpp
#include <hgl/audio/MIDIOrchestraPlayer.h>

using namespace hgl;

MIDIOrchestraPlayer player;
player.SetSoundFont("/path/to.sf2");
player.SetSampleRate(44100);

if(player.Load(OS_TEXT("orchestra.mid")))
{
    player.SetLayout(OrchestraLayout::Standard);
    player.Play();
}
```

## 注意事项

- 推荐使用 FluidSynth 插件获得更完整的多通道支持。
- 多通道同时播放对系统资源要求更高，适合离线或高性能场景。
- `GetChannelSource()` 可进一步应用滤波等 AudioSource 级效果。
