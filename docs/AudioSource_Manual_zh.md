# AudioSource 技术手册

## 概述

`AudioSource` 是单个发声源的控制对象，负责播放控制、空间参数设置以及滤波等效果。它可绑定一个 `AudioBuffer` 并进行播放，支持位置、速度、方向、多普勒、锥形衰减与距离衰减等功能。

## 主要特性

- 播放控制：播放、暂停、继续、停止、重绕。
- 空间属性：位置、速度、方向、锥形角度。
- 距离衰减：距离模型、参考距离、最大距离、Rolloff。
- 多普勒与空气吸收。
- EFX 直达声滤波：低通/高通/带通。

## 关键接口

### 播放控制

- `Play()` / `Play(bool loop)`
- `Pause()` / `Resume()` / `Stop()` / `Rewind()`

### 绑定与生命周期

- `Create()` / `Close()`
- `Link(AudioBuffer*)` / `Unlink()`

### 空间属性

- `SetPosition()` / `GetPosition()`
- `SetVelocity()` / `GetVelocity()`
- `SetDirection()` / `GetDirection()`
- `SetConeAngle()` / `GetAngle()`

### 距离衰减

- `SetDistanceModel()`
- `SetDistance(ref_distance, max_distance)`
- `SetRolloffFactor()`

### 多普勒与空气吸收

- `SetDopplerFactor()` / `SetDopplerVelocity()`
- `SetAirAbsorptionFactor()`

### 滤波器

- `SetLowpassFilter(gain, gain_hf)`
- `SetHighpassFilter(gain, gain_lf)`
- `SetBandpassFilter(gain, gain_lf, gain_hf)`
- `SetFilter(const AudioFilterConfig&)`
- `DisableFilter()`

## 使用流程

1. 创建 `AudioSource`。
2. 通过 `Link(AudioBuffer*)` 绑定缓冲。
3. 设置空间参数/衰减模型/滤波效果(可选)。
4. 调用 `Play()`。

## 示例代码

```cpp
#include <hgl/audio/AudioBuffer.h>
#include <hgl/audio/AudioSource.h>

using namespace hgl;

AudioBuffer buffer(OS_TEXT("fire.wav"), AudioFileType::Wav);
AudioSource source(&buffer);
source.SetPosition(Vector3f(0, 0, 0));
source.SetDistance(1.0f, 50.0f);
source.SetLowpassFilter(1.0f, 0.5f);
source.Play(true);
```

## 注意事项

- 滤波器依赖 OpenAL EFX 支持，若平台不支持则无效。
- 若音源处于播放状态，修改参数会即时影响播放。
- 不建议在多个线程同时访问同一个 `AudioSource`。
