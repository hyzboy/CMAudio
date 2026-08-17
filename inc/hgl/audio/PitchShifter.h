#pragma once

#include<hgl/CoreType.h>
#include<hgl/audio/WSOLAShifter.h>
#include<hgl/audio/LinearResampler.h>

namespace hgl::audio
{
    /**
    * 实时变调器（P1）：变调不变速（pitch shift，音调改变、时长不变）
    *
    * 组合：WSOLAShifter（变速 1/pitch，音调不变）→ LinearResampler（重采样 ×pitch，
    * 音调改变、时长恢复）。这是变声器核心：pitch>1 升调（Helium/女声化），pitch<1 降调。
    *
    * 流式接口：Process 追加输入，ReadOutput 取走输出（输出样本数 ≈ 输入样本数）。
    */
    class PitchShifter
    {
        WSOLAShifter  stretcher;        ///< 变速（时长 /pitch）
        LinearResampler resampler;      ///< 重采样（音调 ×pitch，时长恢复）
        float pitch;

    public:

        PitchShifter();

        /**
        * 初始化
        * @param sample_rate 采样率
        * @param pitch 变调比（0.5=降八度，1.0=不变，2.0=升八度）
        */
        void Init(float sample_rate,float pitch=1.0f);

        void SetPitch(float p);         ///< 设置变调比（0.5-2.0）
        float GetPitch()const{return pitch;}

        void Reset();

        void Process(const float *in,int count);
        int  GetOutputCount()const;     ///< 可取输出样本数（≈ 输入数）
        int  ReadOutput(float *out,int max_count);
    };//class PitchShifter
}//namespace hgl::audio
