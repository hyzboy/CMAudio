#pragma once

#include<hgl/CoreType.h>
#include<vector>

namespace hgl::audio
{
    /**
    * 双二阶滤波器（Direct Form I）
    *   y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
    */
    struct BiquadFilter
    {
        float b0, b1, b2, a1, a2;   ///< 滤波系数（a0 已归一化为 1）
        float x1, x2, y1, y2;       ///< 状态

        BiquadFilter();
        void  SetCoeffs(float B0, float B1, float B2, float A1, float A2);
        void  Reset();
        float Process(float x);
    };//struct BiquadFilter

    /**
    * EBU R128 K-weighting 滤波器（两级级联）
    *   系数为 48kHz 采样率标准系数（ITU-R BS.1770-4）
    *   Stage 1：高通（衰减低频）
    *   Stage 2：高频 shelf（+4dB 抬升高频，约 1kHz 起效，1kHz 处约 +0.7dB）
    */
    class KWeightingFilter
    {
        BiquadFilter stage1;
        BiquadFilter stage2;

    public:
        KWeightingFilter();
        void  Reset();
        float Process(float x);
    };//class KWeightingFilter

    /**
    * 响度测量（LUFS）
    *   K-weighting -> 分块 RMS -> LUFS = 10*log10(mean_square)
    *   - momentary：最近 400ms
    *   - short-term：最近 3s
    *   - integrated：全程平均
    *   简化说明：绝对门限 -70 LUFS（低于视为静音）；未加 EBU R128 integrated
    *   的 -0.691 LUFS 相对门控偏移（广播级精细测量可后续补）。
    *   仅精确支持 48kHz（K-weighting 标准系数）；其它采样率请先重采样到 48kHz。
    */
    class LoudnessMeter
    {
        struct RingWindow
        {
            std::vector<float> buf;
            int   cap, pos, filled;
            double sum;

            void Init(int capacity);
            void Push(double sq);
            double Mean()const;
        };

        KWeightingFilter k_filter;
        float sample_rate;
        RingWindow momentary;
        RingWindow short_term;
        double integrated_sum;
        int64  integrated_count;

    public:
        LoudnessMeter();

        bool Init(float sample_rate);                   ///< 初始化（仅 48kHz 返回 true）
        void Process(const float *samples, int count);  ///< 喂入 mono float 样本
        float GetMomentaryLUFS()const;                  ///< 400ms 响度
        float GetShortTermLUFS()const;                  ///< 3s 响度
        float GetIntegratedLUFS()const;                 ///< 全程平均响度
        void  Reset();
    };//class LoudnessMeter

    /**
    * 响度归一化
    */
    class LoudnessNormalizer
    {
    public:
        /// 计算使音频达到目标 LUFS 所需的线性增益（非 48kHz 返回 1.0 不改）
        static float ComputeNormalizeGain(const float *samples, int count,
                                          float sample_rate, float target_lufs = -23.0f);
        /// 对样本原地应用线性增益
        static void  ApplyGain(float *samples, int count, float gain);
    };//class LoudnessNormalizer
}//namespace hgl::audio
