# SpatialAudioWorld 技术手册

## 概述

`SpatialAudioWorld` 是空间音频场景管理器，负责管理逻辑音源与物理 `AudioSource` 的分配与调度，并提供混响、距离衰减、频率相关衰减与场景级低通等能力。它适合用于游戏或交互式场景的 3D 声音管理。

## 主要特性

- 逻辑音源池与物理音源池分离，支持自动分配与回收。
- 距离衰减模型与多普勒效果。
- 频率相关衰减(模拟空气高频更快衰减)。
- 场景级低通(用于水下/门后等整体听感)。
- 淡入淡出与重要性分层更新。
- OpenAL EFX 混响预设支持。

## 关键类型

### SpatialAudioSourceConfig

逻辑音源创建参数：

- `buffer`：音频缓冲。
- `position`：初始位置。
- `gain`：音量增益。
- `priority`：优先级(用于调度)。
- `distance_model`：距离模型。
- `rolloff_factor`：距离衰减系数。
- `ref_distance` / `max_distance`：参考与最大距离。
- `loop`：是否循环。
- `doppler_factor`：多普勒强度。
- `air_absorption_factor`：空气吸收因子。

### SceneLowpassConfig

场景级低通参数：

- `enable`：是否启用。
- `gain`：整体增益。
- `gain_hf`：高频增益。

### FrequencyAttenuationConfig

频率相关衰减参数：

- `enable`：是否启用。
- `min_gain_hf`：远距离高频最低保留值。

## 使用流程

1. 创建 `SpatialAudioWorld` 并设置 `AudioListener`。
2. 可选：初始化混响/低通/频率衰减。
3. 创建 `SpatialAudioSource` 并设置位置。
4. 每帧调用 `Update()` 刷新。

## 示例代码

```cpp
#include <hgl/audio/SpatialAudioWorld.h>
#include <hgl/audio/Listener.h>

using namespace hgl;

AudioListener listener;
SpatialAudioWorld world(64, &listener);

world.InitReverb();
world.SetReverbPreset(AudioReverbPreset::Room);
world.EnableReverb(true);

world.SetSceneLowpass(SceneLowpassConfig{true, 1.0f, 0.6f});
world.SetFrequencyAttenuation(FrequencyAttenuationConfig{true, 0.3f});

SpatialAudioSourceConfig cfg;
cfg.buffer = buffer;
cfg.position = Vector3f(0, 0, 0);

SpatialAudioSource *src = world.Create(cfg);
if(src)
    src->Play();

// 每帧调用
world.Update();
```

## 重要限制与行为说明

- 逻辑音源与物理音源数量由 `max_source` 决定。
- 若启用频率相关衰减与场景级低通，会共享同一低通滤波器。
- 若某个 `AudioSource` 自己启用了滤波器，场景级低通不会覆盖它。
- 建议在多线程环境下只通过公开 API 操作，内部已做锁保护。

## 事件回调

可通过重载以下虚函数扩展行为：

- `OnCheckGain`：音量计算回调。
- `OnToMute` / `OnToHear`：音源静音/激活事件。
- `OnContinuedMute` / `OnContinuedHear`：持续静音/可听事件。
- `OnStopped`：音源播放结束事件。

## 使用场景范例

- 3D 游戏音效管理。
- 虚拟环境与空间化音效。
- 大量动态音源的调度与性能优化。
