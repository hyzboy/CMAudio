---
title: "核心引擎"
linkTitle: "核心引擎"
weight: 10
date: 2026-08-15
description: "AudioEngine / AudioManager / AudioPlayer / AudioSource / AudioListener 的职责与用法"
draft: false
---

# 核心引擎

CMAudio 的"核心引擎"层由五个类构成，覆盖从"引擎中枢"到"单个发声源"的完整播放链路：

| 类 | 职责 |
|---|---|
| `AudioEngine` | 引擎中枢：总线树 + 资源管理 + 空间音频世界 + 统一 `Update()` |
| `AudioManager` | 简单音效池：固定数量音源，`Play(filename)` 即插即用 |
| `AudioPlayer` | 独立线程播放器：适合 BGM 等长音频流式播放，支持淡入淡出 |
| `AudioSource` | 发声源（OpenAL source 封装）：位置/增益/循环/滤波/距离衰减 |
| `AudioListener` | 收听者（OpenAL listener 封装）：位置/朝向，3D 音频的"耳朵" |

## AudioEngine（引擎中枢）

引擎构造时自动建立总线树与资源管理器：

```cpp
class AudioEngine
{
    AudioBus master;               // 根总线
    AudioBus *music, *sfx, *ambient, *ui;   // 标准四子总线
    AudioAssetManager *asset_manager;        // 资源管理（引擎持有）
    UnorderedSet<SpatialAudioWorld *> worlds; // 空间音频世界（引擎不持有生命周期）

public:
    AudioBus *GetMaster / GetMusic / GetSFX / GetAmbient / GetUI();  // 取总线
    AudioAssetManager *GetAssetManager();                            // 取资源管理器
    AudioBuffer *Acquire(const os_char *filename);                   // 加载（缓存去重）
    void Release(AudioBuffer *);   void Release(const os_char *);
    bool AcquireAsync(const os_char *filename);                      // 异步预加载
    void AddWorld(SpatialAudioWorld *);   void RemoveWorld(SpatialAudioWorld *);
    void Update(const double &ct=0);   // 每帧驱动：资源上传 + 各世界刷新
};
```

`Update()` 是唯一需要每帧调用的入口，内部依次：
1. 资源管理：把后台解码完成的缓冲上传到 OpenAL 并登记缓存；
2. 各注册的 `SpatialAudioWorld` 刷新（空间音频）。

```cpp
AudioEngine engine;
engine.AcquireAsync(OS_TEXT("bgm.ogg"));   // 后台预加载
while(running) {
    engine.Update(now);                     // 上传 + 刷新
    if(engine.GetAssetManager()->GetPendingCount() == 0)
        engine.Acquire(OS_TEXT("bgm.ogg")); // 命中缓存，零成本
}
```

## AudioManager（简单音效池）

面向"播放一个音效"的最简接口，内部维护一个固定大小的 `AudioSource` 池，自动复用已停止的音源：

```cpp
AudioManager(int count = 8);                // 并发音源数量
bool Play(const os_char *filename, float gain = 1.0f);  // 播放音效
void SetBus(AudioBus *b);                   // 把池中所有音源挂到某总线
```

```cpp
AudioManager sfx(16);
sfx.SetBus(engine.GetSFX());
sfx.Play(OS_TEXT("ui_click.wav"), 0.6f);
sfx.Play(OS_TEXT("explosion.wav"), 1.0f);
```

> 池满时 `Play()` 返回 false。适合短音效；长音乐请用 `AudioPlayer`。

## AudioPlayer（独立线程播放器）

`AudioPlayer` 派生自 `hgl::Thread`，用一个独立线程做流式解码 + 三缓冲上传，
适合 BGM 等独占长音频，支持淡入淡出与循环：

```cpp
AudioPlayer bgm(OS_TEXT("bgm.ogg"));        // 自动按扩展名识别格式
bgm.SetBus(engine.GetMusic());
bgm.SetLoop(true);
bgm.SetGain(0.9f);
bgm.SetFadeTime(1.0, 2.0);                  // 淡入 1s，淡出 2s
bgm.Play(true);                             // 播放（true = 循环）

// 控制
bgm.Pause();  bgm.Resume();  bgm.Stop();
double t = bgm.GetPlayTime();               // 已播放时间（秒）
double total = bgm.GetTotalTime();          // 总时长
PlayState s = bgm.GetPlayState();           // None/Play/Pause/Exit
```

位置/朝向/距离等 3D 属性通过内嵌的 `AudioSource` 转发（`SetPosition` 等），
与 `AudioSource` 用法一致。

## AudioSource（发声源）

`AudioSource` 是 OpenAL source 的封装，一个源绑定一个 `AudioBuffer` 播放：

```cpp
AudioSource src(buffer);                    // 绑定缓冲区
src.SetBus(engine.GetSFX());                // 挂总线
src.SetGain(1.0f);                          // 或 SetGainDB(-6.0f)
src.SetPitch(1.0f);                         // 播放速率（变调）
src.SetLoop(false);
src.SetPosition({0, 0, 0});                 // 3D 位置
src.SetDistance(1.0f, 100.0f);              // 参考/最大距离
src.SetRolloffFactor(1.0f);                 // 距离衰减系数
src.SetConeAngle(ConeAngle{...});           // 锥形方向
src.SetDopplerFactor(1.0f);                 // 多普勒强度
src.SetAirAbsorptionFactor(0.5f);           // 空气吸收（高频衰减）

src.Play();   src.Pause();   src.Resume();  src.Stop();   src.Rewind();

// 滤波器（EFX 低通/高通/带通）
src.SetLowpassFilter(gain, gain_hf);
src.SetHighpassFilter(gain, gain_lf);
src.SetBandpassFilter(gain, gain_lf, gain_hf);
src.DisableFilter();
```

- 状态查询：`IsPlaying()/IsPaused()/IsStopped()/IsNone()`、`GetState()`、`GetPlaybackTime()`。
- `Link(buffer)` / `Unlink()` 可换绑缓冲区。
- `SetBus()` 把源挂到总线，源的有效增益会乘上总线链的有效增益。

## AudioListener（收听者）

3D 音频的"耳朵"，一个场景一个监听者，由 `SpatialAudioWorld` 或 OpenAL 直接使用：

```cpp
AudioListener listener;
listener.SetPosition({0, 0, 0});
listener.SetVelocity({0, 0, 0});
listener.SetOrientation(at, up);            // 朝向（前/上向量）
```

## 层次关系

```text
AudioEngine ── AudioBus(master) ── AudioBus(music/sfx/ambient/ui)
     │                                    ▲
     ├── AudioAssetManager ── AudioBuffer ─┘ (被 AudioSource 绑定)
     └── SpatialAudioWorld* (注册，非持有) ── AudioSource 池
```

`AudioManager` 与 `AudioPlayer` 都是 `AudioSource` 之上的便捷封装：
前者管理一个音源池，后者用独立线程做流式播放。
