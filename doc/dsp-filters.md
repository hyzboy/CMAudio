---
title: "DSP 滤波"
linkTitle: "DSP 滤波"
weight: 70
date: 2026-08-15
description: "BiquadFilter 双二阶滤波器、ParametricEQ 参数化均衡器、AudioEQ 缓冲级兜底"
draft: false
---

# DSP 滤波

CMAudio 的滤波层是纯 CPU 实现，不依赖 OpenAL EFX，是参数化 EQ / 压缩器 / 时域效果的统一地基。

## BiquadFilter（双二阶滤波器）

`BiquadFilter` 实现 RBJ Audio EQ Cookbook 的**全部 8 种**双二阶滤波器类型，
Direct Form I 结构，`a0` 归一化为 1：

```cpp
enum class BiquadType
{
    Lowpass, Highpass, Bandpass, BandpassCSG,   // 带通（恒定峰值 / 恒定裙边）
    Notch, Peaking, LowShelf, HighShelf, Allpass
};

class BiquadFilter
{
    BiquadFilter();
    BiquadFilter(BiquadType type, float sample_rate, float cutoff,
                 float q = 0.7071f, float gain_db = 0.0f);
    void Configure(BiquadType type, float sample_rate, float cutoff,
                   float q = 0.7071f, float gain_db = 0.0f);
    void SetCoeffs(float b0, float b1, float b2, float a1, float a2);  // 直接设系数
    void Reset();                          // 清状态（不改系数）
    float Process(float x);                // 单样本
};
```

两种用法：

```cpp
// 1. 按频率参数配置
BiquadFilter lp(BiquadType::Lowpass, 48000, 1000.0f, 0.7071f);   // 1kHz 低通，Butterworth
float y = lp.Process(x);

// 2. 直接设系数（如 EBU R128 K-weighting 标准系数）
BiquadFilter k;  k.SetCoeffs(b0, b1, b2, a1, a2);
```

**要点**：
- `gain_db` 仅对 Peaking/LowShelf/HighShelf 生效。
- 截止频率会 clamp 到 `[1, 0.49*fs]`；`Q<=0` 回退 0.7071。
- Peaking 用标准 alpha `sin(w0)/(2Q)`；Shelf 用 `sin(w0)/√2`（勿混用）。
- `Q=0.7071`（Butterworth）在截止频率处精确 -3dB（幅度 0.7071），可作测试锚点。

## ParametricEQ（参数化均衡器）

`ParametricEQ` 是 N 段 `BiquadFilter` 的级联，提供增删改查频段：

```cpp
class ParametricEQ
{
    ParametricEQ(float sample_rate);
    void SetSampleRate(float sample_rate);       // 重设采样率（重算所有段系数）
    int  AddBand(BiquadType type, float frequency, float q=0.7071f, float gain_db=0.0f);
    bool SetBand(int index, BiquadType type, float frequency, float q, float gain_db);
    bool RemoveBand(int index);
    void ClearBands();
    int  GetBandCount() const;
    void Reset();
    float Process(float x);                     // 单样本
    void  Process(float *samples, int count);   // 批量原地

    static ParametricEQ Create3Band(float sr,   // 低频 shelf + 中频峰值 + 高频 shelf
        float low_freq, float low_gain_db,
        float mid_freq, float mid_gain_db, float mid_q,
        float high_freq, float high_gain_db);
};
```

```cpp
ParametricEQ eq(48000);
eq.AddBand(BiquadType::LowShelf, 100.0f,  0.7071f, +6.0f);   // 低频抬升
eq.AddBand(BiquadType::Peaking,  1000.0f, 1.0f,    0.0f);    // 中频平坦
eq.AddBand(BiquadType::HighShelf,10000.0f, 0.7071f, -6.0f);  // 高频衰减

eq.Process(buf, count);     // 批量处理
```

- **级联语义 = 增益相乘**（两段 +3dB Peaking 级联 = +6dB）。
- 级联顺序 = 添加顺序（LTI 系统，顺序不影响幅度响应）。
- Peaking 低 Q 带宽宽：`Q=1` +6dB @1kHz 在 500Hz 处仍有 +1.88dB；要"只影响目标频段"用 `Q>=4`。

## AudioEQ（缓冲级 CPU EQ 兜底）

`ApplyEQToPCM` 对整块 PCM 数据原地应用 `ParametricEQ`，是 EFX 不可用时的 CPU 路径：

```cpp
bool ApplyEQToPCM(void *data, uint size, const AudioDataInfo &info, ParametricEQ &eq);
```

- 支持 **int16 / float32 交错数据**，逐声道独立滤波、独立 `Reset`。
- 8bit/int24 等格式返回 false 且不修改数据；band 数为 0 时直通返回 true。

```cpp
AudioDataInfo info = {48000, 2, 16, false, size};
ParametricEQ eq(48000);
eq.AddBand(BiquadType::Peaking, 3000.0f, 1.0f, -3.0f);
ApplyEQToPCM(pcm_data, size, info, eq);    // 原地处理
```

> **语义**：这是**加载时一次性频率整形**（OpenAL 静态 buffer 架构无法播放中实时调参）。
> `AudioBuffer::Load/SetData` 与 `AudioMixer::Mix` 都集成了该路径；真正逐帧实时 EQ 需流式三缓冲。

## AudioFilter（EFX 滤波器配置）

`AudioFilterConfig` 是 OpenAL EFX 滤波器的参数载体（低通/高通/带通），
供 `AudioSource::SetFilter()` 使用：

```cpp
enum class AudioFilterType { None, Lowpass, Highpass, Bandpass };
struct AudioFilterConfig
{
    AudioFilterType filter_type = None;
    float gain = 1.0f, gain_lf = 1.0f, gain_hf = 1.0f;
    bool enable = true;
};
```

`AudioFilterPreset` 提供常用滤波预设配置。
