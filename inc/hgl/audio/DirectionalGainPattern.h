#pragma once

#include<hgl/math/Vector.h>
#include<hgl/type/List.h>
#include<hgl/audio/InterpolationType.h>

namespace hgl
{
    using namespace math;

    namespace audio
    {
        /**
         * 极坐标增益图模式枚举
         * Polar gain pattern presets
         */
        enum class GainPatternType
    {
        Omnidirectional,    ///< 全向 - 各方向增益相同
        Cardioid,           ///< 心形 - 前向增益最大，后向最小（类似单向麦克风）
        Supercardioid,      ///< 超心形 - 更强的前向性
        Hypercardioid,      ///< 超超心形 - 最强的前向性
        Bidirectional,      ///< 双向（8字形） - 前后增益最大，侧向最小
        Custom              ///< 自定义模式
    };

    /**
     * 极坐标增益样本点
     * Polar gain sample point
     */
    struct PolarGainSample
    {
        float angle;        ///< 角度（度数，0-360）
        float gain;         ///< 增益值（0.0-1.0）

        PolarGainSample() : angle(0), gain(1.0f) {}
        PolarGainSample(float a, float g) : angle(a), gain(g) {}

        bool operator < (const PolarGainSample &rhs) const
        {
            return angle < rhs.angle;
        }
    };

    /**
     * 方向性增益图类
     * Directional gain pattern class
     * 
     * 用于模拟复杂的方向性声场，支持极坐标增益图
     * Simulates complex directional sound fields using polar gain patterns
     */
    class DirectionalGainPattern
    {
    private:

        GainPatternType pattern_type;
        InterpolationType interpolation_type;   ///< 插值算法类型
        List<PolarGainSample> samples;          ///< 极坐标样本点（已按角度排序）
        
        void InitializePreset(GainPatternType type);
        float InterpolateGain(float angle) const;

    public:

        DirectionalGainPattern();
        DirectionalGainPattern(GainPatternType type);

        /**
         * 设置预定义模式
         * Set predefined pattern
         */
        void SetPattern(GainPatternType type);

        /**
         * 设置自定义极坐标增益图
         * Set custom polar gain pattern
         * @param samples_array 样本点数组
         * @param count 样本点数量
         */
        void SetCustomPattern(const PolarGainSample *samples_array, int count);

        /**
         * 设置插值算法类型
         * Set interpolation algorithm type
         * @param type 插值类型
         */
        void SetInterpolationType(InterpolationType type) { interpolation_type = type; }

        /**
         * 获取插值算法类型
         * Get interpolation algorithm type
         */
        InterpolationType GetInterpolationType() const { return interpolation_type; }

        /**
         * 计算指定方向的增益
         * Calculate gain for specified direction
         * @param source_direction 音源朝向（归一化向量）
         * @param to_listener 从音源指向监听者的向量（归一化向量）
         * @return 增益系数（0.0-1.0）
         */
        float CalculateGain(const Vector3f &source_direction, const Vector3f &to_listener) const;

        /**
         * 获取当前模式类型
         * Get current pattern type
         */
        GainPatternType GetPatternType() const { return pattern_type; }

        /**
         * 是否启用（非全向）
         * Is enabled (not omnidirectional)
         */
        bool IsEnabled() const { return pattern_type != GainPatternType::Omnidirectional; }
    };
    }//namespace audio
}//namespace hgl
