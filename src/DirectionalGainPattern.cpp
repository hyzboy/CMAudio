#include<hgl/audio/DirectionalGainPattern.h>
#include<hgl/audio/InterpolationType.h>
#include<hgl/math/Math.h>
#include<algorithm>

namespace hgl
{
    namespace audio
    {
    DirectionalGainPattern::DirectionalGainPattern()
    {
        pattern_type = GainPatternType::Omnidirectional;
        interpolation_type = InterpolationType::Cosine;  // 默认使用余弦插值，更适合音频
    }

    DirectionalGainPattern::DirectionalGainPattern(GainPatternType type)
    {
        interpolation_type = InterpolationType::Cosine;  // 默认使用余弦插值
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

        // 根据插值类型选择不同的插值方法
        if (interpolation_type == InterpolationType::Linear)
        {
            // 线性插值（2点）
            if (idx == 0)
            {
                // angle小于第一个样本点，使用最后一个和第一个样本点插值（跨越360°边界）
                const PolarGainSample &last = samples[count - 1];
                const PolarGainSample &first = samples[0];
                
                float angle_range = (360.0f - last.angle) + first.angle;
                float t = (angle + (360.0f - last.angle)) / angle_range;
                
                return Interpolation::Linear(last.gain, first.gain, t);
            }
            else if (idx == count)
            {
                // angle大于最后一个样本点，使用最后一个和第一个样本点插值（跨越360°边界）
                const PolarGainSample &last = samples[count - 1];
                const PolarGainSample &first = samples[0];
                
                float angle_range = (360.0f - last.angle) + first.angle;
                float t = (angle - last.angle) / angle_range;
                
                return Interpolation::Linear(last.gain, first.gain, t);
            }
            else
            {
                // 在两个样本点之间插值
                const PolarGainSample &prev = samples[idx - 1];
                const PolarGainSample &next = samples[idx];
                
                float t = (angle - prev.angle) / (next.angle - prev.angle);
                
                return Interpolation::Linear(prev.gain, next.gain, t);
            }
        }
        else if (interpolation_type == InterpolationType::Cosine)
        {
            // 余弦插值（2点）- 提供更平滑的过渡
            if (idx == 0)
            {
                const PolarGainSample &last = samples[count - 1];
                const PolarGainSample &first = samples[0];
                
                float angle_range = (360.0f - last.angle) + first.angle;
                float t = (angle + (360.0f - last.angle)) / angle_range;
                
                return Interpolation::Cosine(last.gain, first.gain, t);
            }
            else if (idx == count)
            {
                const PolarGainSample &last = samples[count - 1];
                const PolarGainSample &first = samples[0];
                
                float angle_range = (360.0f - last.angle) + first.angle;
                float t = (angle - last.angle) / angle_range;
                
                return Interpolation::Cosine(last.gain, first.gain, t);
            }
            else
            {
                const PolarGainSample &prev = samples[idx - 1];
                const PolarGainSample &next = samples[idx];
                
                float t = (angle - prev.angle) / (next.angle - prev.angle);
                
                return Interpolation::Cosine(prev.gain, next.gain, t);
            }
        }
        else if (interpolation_type == InterpolationType::Cubic || interpolation_type == InterpolationType::Hermite)
        {
            // 三次/Hermite插值（4点）- 最平滑的过渡
            // 需要获取4个点：v0, v1(prev), v2(next), v3
            
            int idx_prev = (idx == 0) ? 0 : (idx - 1);
            int idx_curr = (idx == count) ? (count - 1) : idx;
            int idx_next = (idx_curr + 1) % count;
            int idx_prev_prev = (idx_prev == 0) ? (count - 1) : (idx_prev - 1);
            
            float v0 = samples[idx_prev_prev].gain;
            float v1 = samples[idx_prev].gain;
            float v2 = samples[idx_curr].gain;
            float v3 = samples[idx_next].gain;
            
            // 计算插值参数t
            float t;
            if (idx == 0)
            {
                const PolarGainSample &last = samples[count - 1];
                const PolarGainSample &first = samples[0];
                float angle_range = (360.0f - last.angle) + first.angle;
                t = (angle + (360.0f - last.angle)) / angle_range;
            }
            else if (idx == count)
            {
                const PolarGainSample &last = samples[count - 1];
                const PolarGainSample &first = samples[0];
                float angle_range = (360.0f - last.angle) + first.angle;
                t = (angle - last.angle) / angle_range;
            }
            else
            {
                const PolarGainSample &prev = samples[idx - 1];
                const PolarGainSample &next = samples[idx];
                t = (angle - prev.angle) / (next.angle - prev.angle);
            }
            
            return Interpolation::Interpolate(interpolation_type, v0, v1, v2, v3, t);
        }
        
        // 默认线性插值
        if (idx == 0 || idx == count)
        {
            const PolarGainSample &last = samples[count - 1];
            const PolarGainSample &first = samples[0];
            float angle_range = (360.0f - last.angle) + first.angle;
            float t = (idx == 0) ? (angle + (360.0f - last.angle)) / angle_range : (angle - last.angle) / angle_range;
            return Interpolation::Linear(last.gain, first.gain, t);
        }
        else
        {
            const PolarGainSample &prev = samples[idx - 1];
            const PolarGainSample &next = samples[idx];
            float t = (angle - prev.angle) / (next.angle - prev.angle);
            return Interpolation::Linear(prev.gain, next.gain, t);
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
    }//namespace audio
}//namespace hgl
