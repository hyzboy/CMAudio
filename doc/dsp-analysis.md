---
title: "频谱分析"
linkTitle: "频谱分析"
weight: 100
date: 2026-08-15
description: "AudioAnalysis：FFT、RMS/峰值电平、幅度谱、谱通量、onset 检测"
draft: false
---

# 频谱分析（AudioAnalysis）

`AudioAnalysis` 提供纯 CPU 的频谱分析工具：电平测量、radix-2 FFT、幅度谱、谱通量、onset 检测。

## 电平 / 分贝转换

```cpp
float LinearToDB(float v);   // 20*log10，v<=0 返回 -120
float DBToLinear(float db);  // 10^(db/20)
```

## 电平测量

```cpp
float ComputeRMS(const float *samples, int count);   // 均方根（线性）
float ComputeRMSdB(const float *samples, int count); // RMS（dBFS）
float ComputePeak(const float *samples, int count);  // 峰值（线性）
float ComputePeakdB(const float *samples, int count);// 峰值（dBFS）
```

## FFT（radix-2 Cooley-Tukey）

```cpp
bool FFT(float *real, float *imag, int n);
// 原地复数 FFT，n 必须为 2 的幂
// 变换后：DC 在 [0]，Nyquist 在 [n/2]，正频率在 [1..n/2-1]
// Parseval：sum(|X[k]|^2) == n * sum(|x[i]|^2)
```

## 幅度谱

```cpp
bool ComputeMagnitudeSpectrum(const float *samples, int count,
                              float *magnitude, int bin_count);
// 矩形窗 FFT 幅度谱，magnitude[i] = |X[i]| / n（DC bin == 信号均值）
// count 内部向上取整到 2 的幂补零；实际写入 min(bin_count, nfft/2+1) 个 bin
```

> 未加窗，非整数倍频率会有频谱泄漏；需要减泄漏请调用方自行加窗。
> 用它找"主峰频率"：`peak_bin * sample_rate / nfft`（nfft = 向上取整的 2 的幂）。

## 谱通量与 onset 检测

```cpp
float ComputeSpectralFlux(const float *current, const float *previous, int bin_count);
// 谱通量：当前帧与前一帧幅度谱的半波整流正差之和（能量突增 → onset）

class OnsetDetector
{
    OnsetDetector(float onset_ratio = 1.5f);   // 能量 > 前一帧 * ratio 判为 onset
    bool Detect(float frame_energy);
    void Reset();
};
```

## 典型用法

```cpp
const float *mono = ...;   int n = 48000;

// 电平
float rms = ComputeRMS(mono, n);
float peak_db = ComputePeakdB(mono, n);

// 主频（FFT 主峰）
int nfft = 1; while(nfft < n) nfft <<= 1;
std::vector<float> mag(nfft/2 + 1);
ComputeMagnitudeSpectrum(mono, n, mag.data(), nfft/2 + 1);
int peak_bin = 1;
for(int i=2; i<(int)mag.size(); i++) if(mag[i] > mag[peak_bin]) peak_bin = i;
float dominant_freq = (float)peak_bin * 48000.0f / nfft;

// onset 检测
OnsetDetector det(1.5f);
bool is_onset = det.Detect(ComputeRMS(frame, frame_n));
```
