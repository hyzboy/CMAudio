#include<hgl/audio/PitchShifter.h>

namespace hgl::audio
{
    PitchShifter::PitchShifter()
    {
        pitch=1.0f;
    }

    void PitchShifter::Init(float sr,float p)
    {
        pitch=p;

        if(pitch<0.5f)pitch=0.5f;
        if(pitch>2.0f)pitch=2.0f;

        // 升调 pitch>1：WSOLA 拉长 pitch 倍（音调不变）→ 重采样 1/pitch
        // （压缩回原时长，频率 ×pitch）→ 时长不变、音调 ×pitch
        stretcher.Init(sr,pitch);
        resampler.Init(sr,1.0f/pitch);
    }

    void PitchShifter::SetPitch(float p)
    {
        pitch=p;

        if(pitch<0.5f)pitch=0.5f;
        if(pitch>2.0f)pitch=2.0f;

        stretcher.SetStretch(pitch);
        resampler.SetRatio(1.0f/pitch);
    }

    void PitchShifter::Reset()
    {
        stretcher.Reset();
        resampler.Reset();
    }

    void PitchShifter::Process(const float *in,int count)
    {
        if(!in||count<=0)
            return;

        stretcher.Process(in,count);

        // 把变速器输出全部喂给重采样器
        while(stretcher.GetOutputCount()>0)
        {
            float tmp[1024];

            const int got=stretcher.ReadOutput(tmp,1024);

            if(got<=0)
                break;

            resampler.Process(tmp,got);
        }
    }

    int PitchShifter::GetOutputCount()const
    {
        return resampler.GetOutputCount();
    }

    int PitchShifter::ReadOutput(float *out,int max_count)
    {
        return resampler.ReadOutput(out,max_count);
    }
}//namespace hgl::audio
