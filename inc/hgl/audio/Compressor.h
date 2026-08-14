#pragma once

#include<hgl/CoreType.h>

namespace hgl::audio
{
    /**
    * 动态范围压缩器（P3）
    *
    * feed-forward 峰值检测 + attack/release 指数平滑。
    *   - 输入峰值超过 threshold 的部分按 1:ratio 压缩
    *   - ratio 很大（如 20:1）+ 快 attack ≈ 限制器（limiter）
    *
    * 典型用法：
    *   Compressor comp(48000, {.threshold_db=-20, .ratio=4, .attack_sec=0.01,
    *                            .release_sec=0.1, .makeup_gain_db=6});
    *   每样本 comp.Process(x)，或批量 comp.Process(buf, count);
    */
    class Compressor
    {
    public:
        struct Settings
        {
            float threshold_db = -20.0f;    ///< 阈值（dBFS，超过才压缩）
            float ratio = 4.0f;             ///< 压缩比（1.0=不压缩；20+ ≈ 限制器）
            float attack_sec = 0.010f;      ///< 启动时间（秒，0=瞬时）
            float release_sec = 0.100f;     ///< 释放时间（秒，0=瞬时）
            float makeup_gain_db = 0.0f;    ///< 补偿增益（dB，压缩后整体抬升）
        };

    private:
        Settings settings;
        float sample_rate;
        float gain_reduction_db;    ///< 当前增益衰减（<=0，dB）
        float attack_coeff;         ///< attack 平滑系数（每样本）
        float release_coeff;        ///< release 平滑系数（每样本）
        float makeup_linear;        ///< 补偿增益（线性）

    public:
        Compressor();
        Compressor(float sample_rate, const Settings &s);

        void SetSampleRate(float sample_rate);      ///< 重设采样率（重算平滑系数）
        void Configure(const Settings &s);          ///< 设置参数（重算系数）

        const Settings &GetSettings()const{return settings;}

        void  Reset();                              ///< 清零状态（增益衰减归 0）
        float Process(float x);                     ///< 单样本
        void  Process(float *samples, int count);   ///< 批量原地

        float GetGainReductionDB()const{return gain_reduction_db;}   ///< 当前增益衰减（<=0，0=无压缩）
    };//class Compressor
}//namespace hgl::audio
