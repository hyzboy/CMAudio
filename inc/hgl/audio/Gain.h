#pragma once

#include <cmath>
#include <algorithm>

namespace hgl::audio
{
    /**
     * 线性增益 → 分贝(dB)
     * Linear gain → decibels
     *
     * 公式: dB = 20 * log10(linear)
     * 参考: 1.0 → 0dB, 0.5 → -6.02dB, 0.1 → -20dB, 0.01 → -40dB
     *
     * 说明: 线性增益 0.0 在数学上是 -∞ dB，这里 clamp 到 -120dB
     *       （-120dB 在 16bit 音频中已低于最低有效位，实践中视为静音）。
     */
    inline float GainToDB(const float linear)
    {
        return (linear <= 1e-6f) ? -120.0f : 20.0f * std::log10(linear);
    }

    /**
     * 分贝(dB) → 线性增益
     * Decibels → linear gain
     *
     * 公式: linear = 10^(dB/20)
     * 参考: 0dB → 1.0, -6dB → 0.501, -20dB → 0.1, -40dB → 0.01
     *
     * 说明: 低于 -120dB 视为静音(返回 0.0)，避免下溢。
     */
    inline float DBToGain(const float db)
    {
        return (db <= -120.0f) ? 0.0f : std::pow(10.0f, db / 20.0f);
    }

    /**
     * 线性增益 → 分贝(dB)（double 版本）
     */
    inline double GainToDB(const double linear)
    {
        return (linear <= 1e-9) ? -120.0 : 20.0 * std::log10(linear);
    }

    /**
     * 分贝(dB) → 线性增益（double 版本）
     */
    inline double DBToGain(const double db)
    {
        return (db <= -120.0) ? 0.0 : std::pow(10.0, db / 20.0);
    }
}//namespace hgl::audio
