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
        Hermite         ///< Hermite插值 - 最平滑，考虑切线
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
                default:
                    return Linear(v1, v2, t);
            }
        }
    };
    }//namespace audio
}//namespace hgl
