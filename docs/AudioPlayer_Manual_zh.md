# AudioPlayer 技术手册

## 概述

`AudioPlayer` 是面向单个音频文件的播放器类，内部维护独立线程，适合用于背景音乐等“独占式播放”。它通过 `AudioSource` 实现播放与空间属性控制，并提供自动增益、淡入淡出等能力。

## 主要特性

- 独立播放线程。
- 播放控制：播放/暂停/继续/停止。
- 循环播放与淡入淡出。
- 音量、音调与空间参数控制。
- 自动增益(渐变控制)。

## 关键接口

### 播放控制

- `Load(...)`：从文件或流加载音频。
- `Play(bool loop)` / `Stop()` / `Pause()` / `Resume()`
- `SetLoop(bool)`

### 音量与音调

- `SetGain(float)` / `GetGain()`
- `SetPitch(float)` / `GetPitch()`

### 空间属性

- `SetPosition()` / `SetVelocity()` / `SetDirection()`
- `SetDistance(ref_distance, max_distance)`
- `SetConeAngle()`

### 其它能力

- `SetFadeTime(fade_in, fade_out)`：设置淡入淡出时间。
- `AutoGain(target, duration, cur_time)`：自动增益变化。

## 使用流程

1. 创建 `AudioPlayer`。
2. `Load()` 加载音频文件。
3. `Play()` 播放。
4. 根据需要设置空间参数或淡入淡出。

## 示例代码

```cpp
#include <hgl/audio/AudioPlayer.h>

using namespace hgl;

AudioPlayer player;
if(player.Load(OS_TEXT("bgm.ogg")))
{
    player.SetLoop(true);
    player.SetGain(0.8f);
    player.Play();
}
```

## 注意事项

- `AudioPlayer` 用于单个音频的独占播放，不适合大量并发音源。
- 播放线程会在对象销毁时关闭。
- 空间属性只是对内部 `AudioSource` 的转发设置。
