---
title: "声音事件与动态音乐"
linkTitle: "声音事件与动态音乐"
weight: 40
date: 2026-08-15
description: "SoundEventManager 数据驱动事件、SoundEventConfig、DynamicMusic 动态分层音乐"
draft: false
---

# 声音事件与动态音乐

## 数据驱动：SoundEventManager

`SoundEventManager` 把音频内容策划从代码移到配置文件（TOML），
代码只按**事件名**触发播放，是"策划友好"的数据驱动层。

```cpp
class SoundEventManager
{
    bool LoadFromTOML(const char *filename);    // 加载事件配置
    // 按事件名触发/停止（内部查表 → 随机化 → 挂总线播放）
};
```

### SoundEventConfig（事件配置）

```cpp
struct SoundEventConfig
{
    OSStringList files;                 // 音频文件变体列表（播放时随机选一个）
    float min_gain=1.0f, max_gain=1.0f; // 音量随机化范围
    float min_pitch=1.0f, max_pitch=1.0f;// 音高随机化范围
    float priority=1.0f;                // 优先级（音源调度）
    float reference_distance=1.0f, max_distance=10000.0f, rolloff_factor=1.0f;
    bool  loop=false;
    AudioBusType bus_type=AudioBusType::SFX;   // 分组（Master/Music/SFX/Ambient/UI）
};
```

`AudioBusType` 对应 `AudioEngine` 的标准子总线；`AudioBusTypeFromString/ToString` 做字符串互转。

### TOML 配置示例

```toml
[event.shoot]
files = ["shoot_1.wav", "shoot_2.wav", "shoot_3.wav"]
min_gain = 0.9
max_gain = 1.0
min_pitch = 0.9
max_pitch = 1.1
bus_type = "SFX"

[event.step]
files = ["step_grass.wav", "step_gravel.wav"]
min_gain = 0.4
max_gain = 0.6
bus_type = "SFX"
```

```cpp
SoundEventManager events;
events.LoadFromTOML("sound_events.toml");
events.Play("shoot");        // 随机选变体 + 随机增益/音高
```

## 动态音乐：DynamicMusic

`DynamicMusic` 实现"多层 stem + 游戏状态驱动 crossfade"：多个音乐层（鼓/旋律/贝斯…）
同时播放，游戏状态（探索/战斗/…）决定各层目标音量，切换时平滑过渡。

```cpp
class DynamicMusic
{
    int AddLayer(AudioPlayer *player, float base_gain=1.0f);       // 加层，返回索引
    int AddState(const os_char *name, const float *gains, int count); // 加状态
    bool SetState(const os_char *name, double now, double crossfade=1.0); // 切换状态
    bool SetState(int index, double now, double crossfade=1.0);
    void Update(double now);                                       // 每帧驱动 crossfade
    float GetLayerGain(int layer) const;                           // 某层当前实际增益
    AudioPlayer *GetLayerPlayer(int layer) const;
};
```

`State` 的 `gains` 数组与层一一对应，值为该状态下各层的目标增益（0=静音，1=满）。

```cpp
DynamicMusic music;

music.AddLayer(drums,  1.0f);    // 层 0：鼓
music.AddLayer(bass,   1.0f);    // 层 1：贝斯
music.AddLayer(melody, 1.0f);    // 层 2：旋律

float explore[] = {0.6f, 0.3f, 1.0f};   // 探索：鼓 60%、贝斯 30%、旋律 100%
float combat [] = {1.0f, 1.0f, 1.0f};   // 战斗：全部拉满
music.AddState("explore", explore, 3);
music.AddState("combat",  combat,  3);

music.SetState("explore", now, 2.0);    // 2 秒 crossfade 进入探索状态

while(running) {
    music.Update(now);                  // 每帧平滑过渡各层增益
}
```

> `Layer.player` 可为 nullptr（纯逻辑测试）；层增益通过 `GainRamp` 平滑过渡。
