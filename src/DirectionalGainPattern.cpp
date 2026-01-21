#include<hgl/audio/DirectionalGainPattern.h>
#include<hgl/math/Math.h>
#include<algorithm>

namespace hgl
{
    DirectionalGainPattern::DirectionalGainPattern()
    {
        pattern_type = GainPatternType::Omnidirectional;
    }

    DirectionalGainPattern::DirectionalGainPattern(GainPatternType type)
    {
        SetPattern(type);
    }

    void DirectionalGainPattern::InitializePreset(GainPatternType type)
    {
        samples.Clear();
        pattern_type = type;

        if (type == GainPatternType::Omnidirectional)
        {
            // 全向：所有方向增益为1.0
            return;
        }

        // 为不同模式定义增益曲线
        // 角度0°表示音源正前方，180°表示正后方

        if (type == GainPatternType::Cardioid)
        {
            // 心形：gain = 0.5 + 0.5*cos(θ)
            // 前方(0°)增益1.0，侧向(90°)增益0.5，后方(180°)增益0.0
            for (int angle = 0; angle <= 360; angle += 15)
            {
                float radians = angle * DEG_TO_RAD;
                float gain = 0.5f + 0.5f * hgl_cos(radians);
                samples.Add(PolarGainSample(angle, gain));
            }
        }
        else if (type == GainPatternType::Supercardioid)
        {
            // 超心形：gain = 0.37 + 0.63*cos(θ)
            // 更强的前向性
            for (int angle = 0; angle <= 360; angle += 15)
            {
                float radians = angle * DEG_TO_RAD;
                float gain = 0.37f + 0.63f * hgl_cos(radians);
                samples.Add(PolarGainSample(angle, hgl_max(0.0f, gain)));
            }
        }
        else if (type == GainPatternType::Hypercardioid)
        {
            // 超超心形：gain = 0.25 + 0.75*cos(θ)
            // 最强的前向性
            for (int angle = 0; angle <= 360; angle += 15)
            {
                float radians = angle * DEG_TO_RAD;
                float gain = 0.25f + 0.75f * hgl_cos(radians);
                samples.Add(PolarGainSample(angle, hgl_max(0.0f, gain)));
            }
        }
        else if (type == GainPatternType::Bidirectional)
        {
            // 8字形：gain = |cos(θ)|
            // 前后方向增益最大，侧向增益为0
            for (int angle = 0; angle <= 360; angle += 15)
            {
                float radians = angle * DEG_TO_RAD;
                float gain = hgl_abs(hgl_cos(radians));
                samples.Add(PolarGainSample(angle, gain));
            }
        }

        // 确保样本点按角度排序
        std::sort(samples.begin(), samples.end());
    }

    void DirectionalGainPattern::SetPattern(GainPatternType type)
    {
        InitializePreset(type);
    }

    void DirectionalGainPattern::SetCustomPattern(const PolarGainSample *samples_array, int count)
    {
        samples.Clear();
        pattern_type = GainPatternType::Custom;

        for (int i = 0; i < count; i++)
        {
            samples.Add(samples_array[i]);
        }

        // 按角度排序以便插值查找
        std::sort(samples.begin(), samples.end());
    }

    float DirectionalGainPattern::InterpolateGain(float angle) const
    {
        if (samples.GetCount() == 0)
            return 1.0f;

        if (samples.GetCount() == 1)
            return samples[0].gain;

        // 归一化角度到 [0, 360)
        while (angle < 0) angle += 360.0f;
        while (angle >= 360.0f) angle -= 360.0f;

        // 查找角度所在区间
        int count = samples.GetCount();
        
        // 查找第一个角度大于等于目标角度的样本点
        int idx = 0;
        for (; idx < count; idx++)
        {
            if (samples[idx].angle >= angle)
                break;
        }

        if (idx == 0)
        {
            // angle小于第一个样本点，使用最后一个和第一个样本点插值（跨越360°边界）
            const PolarGainSample &last = samples[count - 1];
            const PolarGainSample &first = samples[0];
            
            float angle_range = (360.0f - last.angle) + first.angle;
            float t = (angle + (360.0f - last.angle)) / angle_range;
            
            return last.gain + t * (first.gain - last.gain);
        }
        else if (idx == count)
        {
            // angle大于最后一个样本点，使用最后一个和第一个样本点插值（跨越360°边界）
            const PolarGainSample &last = samples[count - 1];
            const PolarGainSample &first = samples[0];
            
            float angle_range = (360.0f - last.angle) + first.angle;
            float t = (angle - last.angle) / angle_range;
            
            return last.gain + t * (first.gain - last.gain);
        }
        else
        {
            // 在两个样本点之间插值
            const PolarGainSample &prev = samples[idx - 1];
            const PolarGainSample &next = samples[idx];
            
            float t = (angle - prev.angle) / (next.angle - prev.angle);
            
            return prev.gain + t * (next.gain - prev.gain);
        }
    }

    float DirectionalGainPattern::CalculateGain(const Vector3f &source_direction, const Vector3f &to_listener) const
    {
        if (pattern_type == GainPatternType::Omnidirectional)
            return 1.0f;

        // 计算音源朝向与指向监听者方向之间的夹角
        // 注意：to_listener是从音源指向监听者的向量，
        // 我们需要计算它与source_direction之间的角度

        float dot = source_direction.dot(to_listener);
        
        // 限制dot值在[-1, 1]范围内，避免浮点误差导致acos出错
        dot = hgl_clamp(dot, -1.0f, 1.0f);
        
        float angle_radians = hgl_acos(dot);
        float angle_degrees = angle_radians * RAD_TO_DEG;

        // 使用插值获取该角度的增益
        return InterpolateGain(angle_degrees);
    }
}//namespace hgl
