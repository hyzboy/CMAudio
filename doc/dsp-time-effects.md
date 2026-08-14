---
title: "DSP 时域效果"
linkTitle: "DSP 时域效果"
weight: 90
date: 2026-08-15
description: "DelayLine 延迟线、Echo 回声、Chorus 合唱/Flanger 镶边"
draft: false
---

# DSP 时域效果（TimeEffects）

`TimeEffects.h` 提供 header-only 的时域效果（DSP 热点，inline 便于优化），
全部基于一个环形缓冲 `DelayLine`。

## DelayLine（环形缓冲延迟线）

```cpp
class DelayLine
{
    void Init(int max_delay_samples);   // 分配缓冲
    void Reset();                       // 清零 + 复位写指针
    void Write(float x);                // 写入
    float Read(float delay_samples);           // 整数延迟读取（delay=0 即刚写入）
    float ReadInterpolated(float delay_samples);// 线性插值（支持分数延迟）
};
```

`ReadInterpolated` 支持分数延迟，用于 Chorus/Flanger 的 LFO 调制延迟时间。

## Echo（回声：延迟 + 反馈）

```cpp
class Echo
{
    void Init(float sr, float delay_sec, float feedback=0.0f, float mix=0.5f);
    void Reset();
    float Process(float x);              // 单样本
    void Process(float *samples, int count);
    float GetDelaySeconds() const;  float GetFeedback() const;  float GetMix() const;
};
```

```cpp
Echo echo;
echo.Init(48000, 0.25f, 0.4f, 0.5f);    // 250ms 延迟，40% 反馈，50% 干湿
echo.Process(buf, count);
```

- `feedback=0` 且 `mix=1` 是纯延迟；`feedback>0` 产生逐次衰减的多次回声。
- 输出 = `x·(1-mix) + delayed·mix`，写入 = `x + delayed·feedback`。
- **off-by-one 已内建修正**：先读后写时 `write_index` 指向下一写入位置，故读 `delay_samples-1`。

## Chorus / Flanger（LFO 调制延迟）

`Chorus` 用一个 LFO 调制延迟时间，本质是变延迟读头：

```cpp
class Chorus
{
    void Init(float sr, float base_delay, float lfo_rate, float lfo_depth,
              float feedback=0.0f, float mix=0.5f);
    void Reset();
    float Process(float x);   void Process(float *samples, int count);

    static Chorus CreateChorus(float sr);   // 20ms 延迟 + 0.5Hz LFO ±5ms
    static Chorus CreateFlanger(float sr);  // 5ms 延迟 + 0.2Hz LFO ±3ms + 反馈
};
```

```cpp
Chorus chorus = Chorus::CreateChorus(48000);    // 合唱
Chorus flanger = Chorus::CreateFlanger(48000);  // 镶边
```

- **Chorus**：较长延迟（~20ms）+ 慢 LFO + 少反馈 → 多声部合唱感。
- **Flanger**：短延迟（~5ms）+ 快 LFO + 强反馈 → 扫频梳状滤波。
- 两者本质相同，仅参数不同；`Reset()` 清延迟线 + 复位 LFO 相位。
