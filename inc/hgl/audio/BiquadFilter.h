#pragma once

#include<hgl/CoreType.h>

namespace hgl::audio
{
    /**
    * 双二阶滤波器类型（RBJ Audio EQ Cookbook）
    */
    enum class BiquadType
    {
        Lowpass=0,      ///< 低通
        Highpass,       ///< 高通
        Bandpass,       ///< 带通（恒定 0dB 峰值增益）
        BandpassCSG,    ///< 带通（恒定裙边增益，峰值增益 = Q）
        Notch,          ///< 陷波（带阻）
        Peaking,        ///< 峰值/凹陷 EQ（gain_db 控制 ±dB）
        LowShelf,       ///< 低频搁架（gain_db 控制抬升/衰减）
        HighShelf,      ///< 高频搁架（gain_db 控制抬升/衰减）
        Allpass         ///< 全通（幅度恒定，仅改变相位）
    };//enum class BiquadType

    /**
    * 通用双二阶滤波器（Direct Form I，RBJ Audio EQ Cookbook 系数）
    *   y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
    *
    * 两种用法：
    *   1. Configure(type, sample_rate, cutoff, Q, gain_db) —— 按频率参数计算系数
    *      （截止频率超出 (0, Nyquist) 会被 clamp 到 [1, 0.49*fs]）
    *   2. SetCoeffs(b0,b1,b2,a1,a2) —— 直接设系数（如 EBU R128 K-weighting 标准系数）
    *
    * 这是参数化 EQ / 压缩器 / 时域效果（P2-P4）的统一地基。
    */
    class BiquadFilter
    {
    public:
        BiquadFilter();                                                             ///< 默认构造（直通）
        BiquadFilter(BiquadType type, float sample_rate, float cutoff,
                     float q = 0.7071f, float gain_db = 0.0f);

        void Configure(BiquadType type, float sample_rate, float cutoff,
                       float q = 0.7071f, float gain_db = 0.0f);                    ///< 按频率参数配置并计算系数
        void SetCoeffs(float b0, float b1, float b2, float a1, float a2);           ///< 直接设系数（a0 已归一化为 1）

        void  Reset();                                                              ///< 清零状态（不改变系数）
        float Process(float x);                                                     ///< 处理单个样本

        BiquadType GetType()const{return type;}
        float GetCutoff()const{return cutoff;}
        float GetQ()const{return q;}
        float GetGainDB()const{return gain_db;}

    private:
        float b0, b1, b2, a1, a2;   ///< 滤波系数（a0 归一化为 1）
        float x1, x2, y1, y2;       ///< Direct Form I 状态

        BiquadType type;            ///< 滤波器类型
        float sample_rate;          ///< 采样率（Hz）
        float cutoff;               ///< 截止/中心频率（Hz，实际生效值，已 clamp）
        float q;                    ///< Q 值（带宽）
        float gain_db;              ///< 增益（dB，仅 Peaking/Shelf 生效）
    };//class BiquadFilter
}//namespace hgl::audio
