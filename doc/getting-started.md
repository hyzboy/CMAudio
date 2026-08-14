---
title: "快速开始"
linkTitle: "快速开始"
weight: 1
date: 2026-08-15
description: "CMAudio 的环境准备、构建与最小可运行示例"
draft: false
---

# 快速开始

## 环境准备

- **编译器**：Visual Studio 2026（MSVC 18）+ CMake ≥ 3.10，C++20 标准。
- **依赖**：
  - CMCore（同系列基础库，随 ULRE 一起提供）
  - OpenAL Soft：`vcpkg install openal-soft:x64-windows`
  - libsamplerate / libogg：已内置于源码树，无需单独安装。

### OpenAL 运行库

运行任何走 OpenAL 路径的程序，需把 `OpenAL32.dll` 与 `fmt.dll` 放到运行目录
（`build/out/Windows_64_Debug/`）。无音频设备的环境可用 null 后端加载函数指针：

```cpp
InitOpenALDriver(nullptr);   // 加载 OpenAL 函数指针，不真正打开设备
```

## 构建

CMAudio 通常作为 ULRE 子模块集成构建；也可独立构建：

```bash
cd D:/ULRE
cmake -S . -B build
cmake --build build --target CMAudio --config Debug

# 构建并运行某个示例
cmake --build build --target engine_update_test --config Debug
./build/out/Windows_64_Debug/engine_update_test.exe
```

示例目标的 Visual Studio 工程文件夹统一为 `Examples/CMAudio/<sub_folder>`。

## 最小示例：播放一个音效

```cpp
#include <hgl/audio/AudioEngine.h>
#include <hgl/audio/AudioManager.h>
using namespace hgl::audio;

int main()
{
    // 引擎建立总线树（Master → Music/SFX/Ambient/UI）+ 资源管理器
    AudioEngine engine;

    // 简单音效：AudioManager 维护一个固定大小的音源池
    AudioManager sfx(8);                  // 8 个并发音效源
    sfx.SetBus(engine.GetSFX());          // 挂到 SFX 总线
    sfx.Play(OS_TEXT("shot.wav"), 0.8f);  // 播放音效（增益 0.8）

    // 每帧驱动引擎（资源上传 + 空间音频刷新）
    while(running)
        engine.Update(now);

    return 0;
}
```

## 最小示例：3D 空间音频

```cpp
#include <hgl/audio/AudioEngine.h>
#include <hgl/audio/SpatialAudioWorld.h>
#include <hgl/audio/AudioListener.h>
using namespace hgl::audio;

AudioListener listener;
listener.SetPosition({0, 0, 0});

AudioEngine engine;
SpatialAudioWorld world(64, &listener);   // 最多 64 个空间音源
world.SetBus(engine.GetSFX());
engine.AddWorld(&world);                  // 注册，由 engine.Update() 统一驱动

AudioBuffer *buf = engine.Acquire(OS_TEXT("engine_loop.wav"));

SpatialAudioSourceConfig cfg;
cfg.buffer = buf;
cfg.position = {10.0f, 0, 0};
cfg.ref_distance = 5.0f;
cfg.max_distance = 50.0f;
cfg.loop = true;

SpatialAudioSource *src = world.Create(cfg);
if(src) src->Play();

// 移动监听者
listener.SetPosition({0, 0, 10.0f});

while(running)
    engine.Update(now);                   // world.Update() 会在此被驱动
```

## 下一步

- 想理解架构层次 → [核心引擎](core-engine.md)
- 想控制音量分组 → [音频总线](audio-bus.md)
- 想加载/缓存音频 → [资源管理](asset-management.md)
- 想用数据驱动播放 → [声音事件与动态音乐](sound-events.md)
- 想做信号处理 → [DSP 滤波](dsp-filters.md) 及后续 DSP 手册
