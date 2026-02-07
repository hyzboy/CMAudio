# MIDIPlayer 技术手册

## 概述

`MIDIPlayer` 是面向 MIDI 文件的专业播放器，提供完整的播放控制、通道级控制与多种后端音色库支持。它在内部维护独立播放线程，支持实时调节音量与音调，并可查询通道状态与信息。

## 主要特性

- 独立播放线程，支持播放/暂停/继续/停止。
- 通道控制：乐器、音量、声像、静音、独奏。
- 音色库配置：SoundFont / Bank / BankFile。
- 多通道解码：按通道或全部通道离线解码。
- 空间音频属性：位置/速度/方向。

## 关键能力

### 播放控制

- `Load()` / `Load(io::InputStream*)`：加载 MIDI。
- `Play()` / `Pause()` / `Resume()` / `Stop()`：播放控制。
- `SetLoop(bool)`：循环播放。
- `SetFadeTime(fade_in, fade_out)`：淡入淡出时间。

### 音色与采样率

- `SetSoundFont(path)`：设置 SoundFont。
- `SetBank(bank_id)`：设置音色库ID。
- `SetBankFile(path)`：设置音色库文件。
- `SetSampleRate(rate)`：设置采样率。

### 通道控制

- `SetChannelProgram()`：设置通道乐器。
- `SetChannelVolume()`：设置通道音量。
- `SetChannelPan()`：设置通道声像。
- `SetChannelMute()` / `SetChannelSolo()`：静音/独奏。
- `ResetChannel()` / `ResetAllChannels()`：重置通道。
- `GetChannelInfo()` / `GetChannelActivity()`：查询通道信息与活跃状态。

### 空间属性

- `SetPosition()` / `SetVelocity()` / `SetDirection()`：设置 3D 空间属性。

## 使用流程

1. 创建 `MIDIPlayer`。
2. 设置音色库或 Bank。
3. `Load()` MIDI 文件。
4. 调用 `Play()` 播放。
5. 运行时可实时调整通道或音色。

## 示例代码

```cpp
#include <hgl/audio/MIDIPlayer.h>

using namespace hgl;

MIDIPlayer player;
player.SetSoundFont("/path/to.sf2");
player.SetSampleRate(44100);

if(player.Load(OS_TEXT("music.mid")))
{
    player.SetLoop(true);
    player.Play();
}
```

## 注意事项

- MIDI 后端依赖插件，需确保对应插件可用。
- 多通道解码属于离线操作，适合工具链或批处理。
- 3D 空间参数只影响最终 AudioSource 的空间效果。
