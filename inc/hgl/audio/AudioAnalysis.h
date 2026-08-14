#pragma once

#include<hgl/CoreType.h>

namespace hgl::audio
{
    // ===== 电平 / 分贝转换 =====
    float LinearToDB(float v);                              ///< 线性幅度 -> dB（20*log10，v<=0 返回 -120）
    float DBToLinear(float db);                             ///< dB -> 线性幅度（10^(db/20)）

    // ===== 电平测量 =====
    float ComputeRMS(const float *samples, int count);      ///< 均方根（线性）
    float ComputeRMSdB(const float *samples, int count);    ///< RMS 电平（dBFS）
    float ComputePeak(const float *samples, int count);     ///< 峰值幅度（线性）
    float ComputePeakdB(const float *samples, int count);   ///< 峰值电平（dBFS）

    // ===== FFT =====
    /**
    * 原地复数 radix-2 FFT（Cooley-Tukey）
    *   n 必须为 2 的幂；real/imag 各 n 个样本
    *   变换后 DC 分量在 [0]，Nyquist 在 [n/2]，正频率在 [1..n/2-1]
    *   满足 Parseval：sum(|X[k]|^2) == n * sum(|x[i]|^2)
    *   返回 false 表示参数非法（n 非 2 的幂 / 空指针）
    */
    bool FFT(float *real, float *imag, int n);

    // ===== 频谱 =====
    /**
    * 矩形窗 FFT 幅度谱：magnitude[i] = |X[i]| / n（归一化，DC bin == 信号均值）
    *   count 个样本（内部向上取整到 2 的幂补零）
    *   输出 [0..Nyquist]，实际写入 min(bin_count, nfft/2+1) 个 bin
    *   注：未加窗，非整数倍频率会有频谱泄漏；需要减泄漏请调用方自行加窗
    */
    bool ComputeMagnitudeSpectrum(const float *samples, int count,
                                  float *magnitude, int bin_count);

    // ===== Onset / 节拍检测 =====
    /**
    * 谱通量：当前帧与前一帧幅度谱的半波整流正差之和
    *   值越大表示频谱能量突增越明显（常用于 onset / 节拍检测）
    */
    float ComputeSpectralFlux(const float *current, const float *previous, int bin_count);

    /**
    * 能量包络 onset 检测器
    *   当某帧能量 > 前一帧能量 * ratio 时判定为 onset
    */
    class OnsetDetector
    {
        float previous_energy;      ///< 前一帧能量
        float ratio;                ///< 相对阈值
        bool  has_previous;         ///< 是否已有前一帧

    public:
        OnsetDetector(float onset_ratio = 1.5f);

        bool Detect(float frame_energy);    ///< 检测一帧能量是否为 onset
        void Reset();                       ///< 重置（清空前一帧）
    };//class OnsetDetector
}//namespace hgl::audio
