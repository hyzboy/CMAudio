#pragma once

#include<hgl/math/Math.h>

namespace hgl
{
    namespace audio
    {
        /**
         * 插值算法类型枚举
         * Interpolation algorithm type enum
         */
        enum class InterpolationType
    {
        Linear,         ///< 线性插值 - 最快，但可能产生突变
        Cosine,         ///< 余弦插值 - 平滑过渡，适合音频
        Cubic,          ///< 三次插值 - 更平滑，需要4个点
        Hermite,        ///< Hermite插值 - 最平滑，考虑切线
        EqualPower,     ///< 等功率插值 - 用于音频交叉淡化，保持总能量恒定
        Exponential,    ///< 指数插值 - 模拟人耳感知的音量变化（dB刻度）
        SCurve          ///< S曲线插值 - 平滑的ease-in/ease-out，类似smoothstep
    };

    /**
     * 插值算法工具类
     * Interpolation utilities
     */
    class Interpolation
    {
    public:
        /**
         * 线性插值
         * Linear interpolation
         * @param v0 起始值
         * @param v1 结束值
         * @param t 插值参数 [0, 1]
         * @return 插值结果
         */
        static inline float Linear(float v0, float v1, float t)
        {
            return v0 + t * (v1 - v0);
        }

        /**
         * 余弦插值
         * Cosine interpolation
         * 使用余弦函数创建平滑的S曲线过渡
         * Uses cosine function to create smooth S-curve transition
         * @param v0 起始值
         * @param v1 结束值
         * @param t 插值参数 [0, 1]
         * @return 插值结果
         */
        static inline float Cosine(float v0, float v1, float t)
        {
            float t2 = (1.0f - hgl_cos(t * HGL_PI)) * 0.5f;
            return Linear(v0, v1, t2);
        }

        /**
         * 三次插值
         * Cubic interpolation
         * 使用4个点进行三次多项式插值
         * Uses 4 points for cubic polynomial interpolation
         * @param v0 前一个点
         * @param v1 起始值
         * @param v2 结束值
         * @param v3 后一个点
         * @param t 插值参数 [0, 1]
         * @return 插值结果
         */
        static inline float Cubic(float v0, float v1, float v2, float v3, float t)
        {
            float t2 = t * t;
            float a0 = v3 - v2 - v0 + v1;
            float a1 = v0 - v1 - a0;
            float a2 = v2 - v0;
            float a3 = v1;

            return a0 * t * t2 + a1 * t2 + a2 * t + a3;
        }

        /**
         * Hermite插值
         * Hermite interpolation
         * 考虑切线的平滑插值，参数控制张力和偏移
         * Smooth interpolation considering tangents with tension and bias control
         * @param v0 前一个点
         * @param v1 起始值
         * @param v2 结束值
         * @param v3 后一个点
         * @param t 插值参数 [0, 1]
         * @param tension 张力 (0=松弛, 1=紧绷) 默认0
         * @param bias 偏移 (-1到1) 默认0
         * @return 插值结果
         */
        static inline float Hermite(float v0, float v1, float v2, float v3, float t,
                                   float tension = 0.0f, float bias = 0.0f)
        {
            float t2 = t * t;
            float t3 = t2 * t;

            float m0 = (v1 - v0) * (1.0f + bias) * (1.0f - tension) * 0.5f +
                      (v2 - v1) * (1.0f - bias) * (1.0f - tension) * 0.5f;
            float m1 = (v2 - v1) * (1.0f + bias) * (1.0f - tension) * 0.5f +
                      (v3 - v2) * (1.0f - bias) * (1.0f - tension) * 0.5f;

            float a0 =  2.0f * t3 - 3.0f * t2 + 1.0f;
            float a1 =         t3 - 2.0f * t2 + t;
            float a2 =         t3 -        t2;
            float a3 = -2.0f * t3 + 3.0f * t2;

            return a0 * v1 + a1 * m0 + a2 * m1 + a3 * v2;
        }

        /**
         * 等功率插值
         * Equal Power interpolation
         * 用于音频交叉淡化，保持总功率恒定：gain₁² + gain₂² = 1
         * Used for audio crossfading, maintains constant total power: gain₁² + gain₂² = 1
         * 适用于两个音频信号的平滑过渡
         * Ideal for smooth transitions between two audio signals
         * @param v0 起始值
         * @param v1 结束值
         * @param t 插值参数 [0, 1]
         * @return 插值结果
         */
        static inline float EqualPower(float v0, float v1, float t)
        {
            // 使用余弦函数确保功率守恒
            // Use cosine function to ensure power conservation
            // fade_out: cos(t * π/2), fade_in: sin(t * π/2)
            float angle = t * HGL_PI * 0.5f;  // 0 to π/2
            float fade_out = hgl_cos(angle);
            float fade_in = hgl_sin(angle);
            return v0 * fade_out + v1 * fade_in;
        }

        /**
         * 指数插值
         * Exponential interpolation
         * 模拟人耳对音量的感知（对数/dB刻度）
         * Simulates human perception of volume (logarithmic/dB scale)
         * 适用于音量渐变，更符合听觉感受
         * Ideal for volume fading, matches auditory perception
         * @param v0 起始值
         * @param v1 结束值
         * @param t 插值参数 [0, 1]
         * @return 插值结果
         */
        static inline float Exponential(float v0, float v1, float t)
        {
            // 使用指数曲线模拟对数音量感知
            // Use exponential curve to simulate logarithmic volume perception
            // 避免v0或v1为0导致的问题
            // Avoid issues when v0 or v1 is 0
            if (v0 < 1e-6f) v0 = 1e-6f;
            if (v1 < 1e-6f) v1 = 1e-6f;
            
            // 对数空间插值，然后转回线性空间
            // Interpolate in log space, then convert back to linear
            float log_v0 = hgl_log(v0);
            float log_v1 = hgl_log(v1);
            float log_result = log_v0 + t * (log_v1 - log_v0);
            return hgl_exp(log_result);
        }

        /**
         * S曲线插值（Smoothstep）
         * S-Curve interpolation (Smoothstep)
         * 平滑的ease-in/ease-out曲线，在起止点速度为0
         * Smooth ease-in/ease-out curve with zero velocity at endpoints
         * 使用3x²-2x³公式，比余弦插值更平滑
         * Uses 3x²-2x³ formula, smoother than cosine interpolation
         * @param v0 起始值
         * @param v1 结束值
         * @param t 插值参数 [0, 1]
         * @return 插值结果
         */
        static inline float SCurve(float v0, float v1, float t)
        {
            // Smoothstep函数：3t² - 2t³
            // Smoothstep function: 3t² - 2t³
            // 在t=0和t=1处导数为0，保证平滑
            // Derivative is 0 at t=0 and t=1, ensuring smoothness
            float t2 = t * t;
            float t3 = t2 * t;
            float smooth_t = 3.0f * t2 - 2.0f * t3;
            return Linear(v0, v1, smooth_t);
        }

        /**
         * 通用插值函数（2点）
         * Generic interpolation function (2 points)
         * @param type 插值类型
         * @param v0 起始值
         * @param v1 结束值
         * @param t 插值参数 [0, 1]
         * @return 插值结果
         */
        static inline float Interpolate(InterpolationType type, float v0, float v1, float t)
        {
            switch (type)
            {
                case InterpolationType::Linear:
                    return Linear(v0, v1, t);
                case InterpolationType::Cosine:
                    return Cosine(v0, v1, t);
                case InterpolationType::EqualPower:
                    return EqualPower(v0, v1, t);
                case InterpolationType::Exponential:
                    return Exponential(v0, v1, t);
                case InterpolationType::SCurve:
                    return SCurve(v0, v1, t);
                default:
                    return Linear(v0, v1, t);  // Fallback to linear for 2-point methods
            }
        }

        /**
         * 通用插值函数（4点）
         * Generic interpolation function (4 points)
         * @param type 插值类型
         * @param v0 前一个点
         * @param v1 起始值
         * @param v2 结束值
         * @param v3 后一个点
         * @param t 插值参数 [0, 1]
         * @return 插值结果
         */
        static inline float Interpolate(InterpolationType type, float v0, float v1, float v2, float v3, float t)
        {
            switch (type)
            {
                case InterpolationType::Linear:
                    return Linear(v1, v2, t);
                case InterpolationType::Cosine:
                    return Cosine(v1, v2, t);
                case InterpolationType::Cubic:
                    return Cubic(v0, v1, v2, v3, t);
                case InterpolationType::Hermite:
                    return Hermite(v0, v1, v2, v3, t);
                case InterpolationType::EqualPower:
                    return EqualPower(v1, v2, t);
                case InterpolationType::Exponential:
                    return Exponential(v1, v2, t);
                case InterpolationType::SCurve:
                    return SCurve(v1, v2, t);
                default:
                    return Linear(v1, v2, t);
            }
        }
    };
    }//namespace audio
}//namespace hgl
