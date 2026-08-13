#pragma once

#include <algorithm>
#include <hgl/audio/InterpolationType.h>

namespace hgl::audio
{
    /**
     * 计算淡入淡出增益因子
     * Calculate fade-in / fade-out gain factor
     *
     * 输入播放位置 t(秒)，返回 [0,1] 的增益系数：
     * - t < fade_in:                淡入段，0 → 1
     * - t > total - fade_out:       淡出段，1 → 0
     * - 其余:                       1.0
     *
     * @param t        当前播放位置(秒)
     * @param fade_in  淡入时长(秒)，<=0 表示禁用
     * @param fade_out 淡出时长(秒)，<=0 表示禁用
     * @param total    音频总时长(秒)
     * @param type     插值类型(默认 Linear)
     */
    inline float FadeFactor(const double t,
                            const double fade_in,
                            const double fade_out,
                            const double total,
                            const InterpolationType type=InterpolationType::Linear)
    {
        if(fade_in>0.0&&t<fade_in)
            return Interpolation::Interpolate(type,0.0f,1.0f,float(std::clamp(t/fade_in,0.0,1.0)));

        if(fade_out>0.0&&t>total-fade_out)
            return Interpolation::Interpolate(type,1.0f,0.0f,float(std::clamp((t-(total-fade_out))/fade_out,0.0,1.0)));

        return 1.0f;
    }

    /**
     * 增益斜坡(用于自动增益/淡入淡出过渡)
     * 在一段时间内将增益从 start_gain 过渡到 end_gain
     */
    struct GainRamp
    {
        bool   active=false;                               ///< 是否处于过渡中
        double start_time=0.0;                             ///< 起始时间(秒)
        double duration=0.0;                               ///< 过渡时长(秒)
        float  start_gain=0.0f;                            ///< 起始增益
        float  end_gain=0.0f;                              ///< 目标增益
        InterpolationType type=InterpolationType::Linear;  ///< 插值类型

        /**
         * 开始一次增益过渡
         * @param now  当前时间(秒)
         * @param from 起始增益
         * @param to   目标增益
         * @param dur  过渡时长(秒)
         */
        void Start(const double now,const float from,const float to,const double dur)
        {
            active=true;
            start_time=now;
            duration=(dur>0.0)?dur:0.0;
            start_gain=from;
            end_gain=to;
        }

        /**
         * 计算当前增益
         * @param now 当前时间(秒)
         * @param out 输出当前增益
         * @return true=仍在过渡中; false=过渡已结束(out=end_gain, active 已置 false)
         */
        bool Evaluate(const double now,float &out)
        {
            if(!active)
            {
                out=end_gain;
                return false;
            }

            if(duration<=0.0||now>=start_time+duration)
            {
                active=false;
                out=end_gain;
                return false;
            }

            out=Interpolation::Interpolate(type,start_gain,end_gain,float((now-start_time)/duration));
            return true;
        }
    };
}//namespace hgl::audio
