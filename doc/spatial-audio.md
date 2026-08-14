---
title: "空间音频"
linkTitle: "空间音频"
weight: 60
date: 2026-08-15
description: "SpatialAudioWorld 3D 场景、方向性、多普勒、频率衰减、混响预设"
draft: false
---

# 空间音频（SpatialAudioWorld）

`SpatialAudioWorld` 管理一个 3D 音频场景：维护一批逻辑发声源 `SpatialAudioSource`，
每帧根据监听者位置计算距离衰减、方向性、多普勒、频率衰减，并驱动底层 `AudioSource`。
支持对象池、线程安全、混响（OpenAL Soft 预设）与事件回调。

## 创建与注册

```cpp
SpatialAudioWorld(int max_source, AudioListener *listener);   // 最大空间音源数
void SetBus(AudioBus *b);        // 场景内所有空间音源挂到某总线
void SetListener(AudioListener *al);
int  Update(const double &ct=0); // 刷新，返回仍在发声的音源数量
```

```cpp
AudioListener listener;
SpatialAudioWorld world(64, &listener);
world.SetBus(engine.GetSFX());
engine.AddWorld(&world);          // 注册后由 engine.Update() 统一驱动
```

> 引擎 `AddWorld/RemoveWorld` 不持有 world 的生命周期，调用方负责销毁。

## 创建与操控音源

```cpp
struct SpatialAudioSourceConfig
{
    AudioBuffer *buffer = nullptr;
    Vector3f position = {0,0,0};
    float gain = 1.0f, priority = 1.0f;
    uint distance_model = 0;        // OpenAL 距离模型
    float rolloff_factor = 1.0f;
    float ref_distance = 1.0f, max_distance = 10000.0f;
    bool loop = false;
    float doppler_factor = 0.0f, air_absorption_factor = 0.0f;
};

SpatialAudioSource *Create(const SpatialAudioSourceConfig &config);
void Delete(SpatialAudioSource *);
void Clear();
```

`SpatialAudioSource` 提供：

```cpp
src->Play(double play_time=0);       // 请求播放（0=可闻时才开始）
src->Stop();
src->MoveTo(const Vector3f &pos, const double &ct);  // 移动（记录上一位置供速度计算）
src->GetGain(const AudioListener *l) const;          // 距离衰减 + 方向性
bool src->IsPlaying() const;
const Vector3f &GetPosition() const;
```

## 方向性增益图

`DirectionalGainPattern`（极坐标增益图）用于比 OpenAL 锥形更复杂的方向性建模，
配合 `ConeAngle`：

```cpp
enum class GainPatternType { /* 预定义模式 */ };

void SetDirectionalPattern(SpatialAudioSource *src, GainPatternType type);
void SetCustomDirectionalPattern(SpatialAudioSource *src, const PolarGainSample *samples, int count);
```

## 频率相关衰减与场景低通

模拟"空气对高频衰减更快"与整体场景滤波：

```cpp
bool InitFrequencyAttenuation();   void CloseFrequencyAttenuation();
bool EnableFrequencyAttenuation(bool);
bool SetFrequencyAttenuation(const FrequencyAttenuationConfig &); // min_gain_hf=远距离高频保留值

bool EnableSceneLowpass(bool);
bool SetSceneLowpass(const float gain, const float gain_hf);
bool SetSceneLowpass(const SceneLowpassConfig &);   // {enable, gain, gain_hf}
void DisableSceneLowpass();
```

## 混响（OpenAL Soft 预设）

`AudioReverbPreset` 提供 113 个 OpenAL Soft 官方预设：

```cpp
bool InitReverb();                    // 初始化混响系统
bool SetReverbPreset(AudioReverbPreset preset);   // 设置预设
bool EnableReverb(bool);              // 启用/禁用
void CloseReverb();
```

## 事件回调

`SpatialAudioWorld` 提供可重载的听觉事件钩子，用于触发游戏逻辑：

```cpp
virtual float OnCheckGain(SpatialAudioSource *);        // 计算音源当前可闻增益
virtual void  OnToMute(SpatialAudioSource *);           // 由可闻 → 不可闻
virtual void  OnToHear(SpatialAudioSource *);           // 由不可闻 → 可闻
virtual void  OnContinuedMute(SpatialAudioSource *);
virtual void  OnContinuedHear(SpatialAudioSource *);
virtual bool  OnStopped(SpatialAudioSource *);          // 播放结束，返回 true=可释放
```

## 完整示例

```cpp
AudioListener listener;  listener.SetPosition({0,0,0});
SpatialAudioWorld world(64, &listener);
world.SetBus(engine.GetSFX());
world.SetDistance(1.0f, 100.0f);
world.SetSceneLowpass(SceneLowpassConfig{true, 1.0f, 0.5f});
world.InitReverb();  world.SetReverbPreset(AudioReverbPreset::Cave);  world.EnableReverb(true);

SpatialAudioSourceConfig cfg;
cfg.buffer = engine.Acquire(OS_TEXT("bird.wav"));
cfg.position = {10.0f, 2.0f, 0};
cfg.ref_distance = 5.0f;  cfg.max_distance = 60.0f;  cfg.loop = true;
SpatialAudioSource *src = world.Create(cfg);
if(src) src->Play();

while(running) {
    listener.SetPosition(camera_pos);
    world.Update(now);      // 或由 engine.Update() 统一驱动
}
```

## 线程安全说明

- `Create/Delete/Clear/Update/SetListener/SetDistance/InitReverb/...` 等公共 API 线程安全（内部互斥锁）。
- `SpatialAudioSource` 的成员方法（`Play/Stop/MoveTo`）**非线程安全**，应在持锁或单线程下调用。
