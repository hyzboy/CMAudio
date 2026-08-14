---
title: "DSP 动态与响度"
linkTitle: "DSP 动态与响度"
weight: 80
date: 2026-08-15
description: "Compressor 压缩/限制/侧链，LoudnessNormalizer 响度测量与归一化"
draft: false
---

# DSP 动态与响度

动态范围处理与响度管理是纯 CPU 实现：`Compressor`（压缩/限制/侧链）与
`LoudnessNormalizer`（EBU R128 LUFS 测量 + 归一化 + 峰值限制）。

## Compressor（动态范围压缩器）

feed-forward 峰值检测 + attack/release 指数平滑：

```cpp
struct Compressor::Settings
{
    float threshold_db = -20.0f;   // 阈值（dBFS，超过才压缩）
    float ratio = 4.0f;            // 压缩比（1.0=不压缩；20+ ≈ 限制器）
    float attack_sec = 0.010f;     // 启动时间（秒，0=瞬时）
    float release_sec = 0.100f;    // 释放时间（秒，0=瞬时）
    float makeup_gain_db = 0.0f;   // 补偿增益（dB）
};

Compressor comp(48000, {.threshold_db=-20, .ratio=4,
                        .attack_sec=0.01, .release_sec=0.1, .makeup_gain_db=6});
float y = comp.Process(x);          // 单样本
comp.Process(buf, count);           // 批量原地
comp.Reset();                       // 清零状态
float gr = comp.GetGainReductionDB(); // 当前增益衰减（<=0）
```

**压缩特性**：输入峰值超过 threshold 的部分按 `1:ratio` 压缩。稳态参考：
ratio=4:1、超阈值 10dB → 增益衰减 -7.5dB；ratio=20 + 快 attack + threshold=0dB ≈ 限制器。

### 侧链用法

```cpp
float UpdateFromLevel(float level);   // 用外部电平（0..1）驱动，返回增益（不含 makeup）
```

`UpdateFromLevel` 与 `Process` 共享检测/平滑逻辑，但只更新增益衰减并返回线性增益，
用于 sidechain（详见 [音频总线](audio-bus.md) 的侧链压缩 Duck）。

## LoudnessNormalizer（响度测量与归一化）

基于 EBU R128（ITU-R BS.1770-4）：

- **KWeightingFilter**：两级 K-weighting（stage1 高通 + stage2 高频 shelf +4dB）。
  **注意**：1kHz 处增益是 **+0.7dB**（非 0dB），满幅 1kHz 正弦的 LUFS 理论值为 **-2.31**，不是 -3.01。
- **LoudnessMeter**：momentary（400ms）/ short-term（3s）/ integrated（全程）三档 LUFS。

```cpp
class LoudnessMeter
{
    bool Init(float sample_rate);             // 仅精确支持 48kHz
    void Process(const float *samples, int count);  // 喂 mono float 样本
    float GetMomentaryLUFS() const;           // 400ms
    float GetShortTermLUFS() const;           // 3s
    float GetIntegratedLUFS() const;          // 全程
    void Reset();
};
```

### 归一化

```cpp
class LoudnessNormalizer
{
    // 计算使音频达到目标 LUFS 所需的线性增益（非 48kHz 返回 1.0）
    static float ComputeNormalizeGain(const float *samples, int count,
                                      float sample_rate, float target_lufs = -23.0f);
    static void ApplyGain(float *samples, int count, float gain);   // 原地增益

    // 峰值限制（true-peak 兜底）：超 limit_peak 部分用高比率 limiter 压回
    static void ApplyPeakLimiter(float *samples, int count, float sample_rate,
                                 float limit_peak = 1.0f);
    // 归一化到目标 LUFS + limiter 兜底
    static void NormalizeWithLimiter(float *samples, int count, float sample_rate,
                                     float target_lufs = -23.0f, float limit_peak = 1.0f);
};
```

```cpp
// 测量
LoudnessMeter meter;
meter.Init(48000);
meter.Process(mono, n);
float lufs = meter.GetIntegratedLUFS();

// 归一化到 -23 LUFS 并限制峰值
LoudnessNormalizer::NormalizeWithLimiter(mono, n, 48000, -23.0f, 1.0f);
```

**限制器局限**：1ms attack 对 1kHz 正弦（周期=1ms）无法完全收敛，
峰值只能压到 ~1.28（非 1.0）；真正的 true-peak 限制需 lookahead 前瞻。
`ApplyPeakLimiter` 是"兜底"而非精确 brickwall。
