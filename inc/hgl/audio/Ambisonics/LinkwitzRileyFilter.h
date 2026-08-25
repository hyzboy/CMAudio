#pragma once

#include<hgl/CoreType.h>
#include<cmath>
#include<vector>

namespace hgl::audio
{
    /**
    * Linkwitz-Riley 分频滤波器（R2 移植，源自 Amplitude Audio SDK，Apache 2.0）
    *
    * 每通道两级级联 Butterworth 低通/高通（Direct Form I，RBJ cookbook 系数），
    * 用于 Ambisonic 心理声学校准的高低频分裂。
    */
    class LinkwitzRileyFilter
    {
        struct BiquadCoefficients
        {
            float b0,b1,b2,a1,a2;
        };

        struct BiquadState
        {
            float x1,x2,y1,y2;
        };

        static constexpr float kButterworthQ=0.7071067811865476f;

    public:
        LinkwitzRileyFilter()
        {
            for(int i=0;i<2;i++)
            {
                lpCoeffs[i]={ 1.0f,0.0f,0.0f,0.0f,0.0f };
                hpCoeffs[i]={ 1.0f,0.0f,0.0f,0.0f,0.0f };
            }
        }

        bool Configure(uint32 channel_count,uint32 sample_rate,float crossover_frequency)
        {
            if(channel_count==0||sample_rate==0||crossover_frequency<=0.0f)
                return false;

            if(crossover_frequency>=static_cast<float>(sample_rate)/2.0f)
                return false;

            channelCount=channel_count;
            sampleRate=sample_rate;
            crossoverFrequency=crossover_frequency;

            for(int i=0;i<2;i++)
            {
                ComputeLowPassCoefficients(crossover_frequency,(float)sample_rate,lpCoeffs[i]);
                ComputeHighPassCoefficients(crossover_frequency,(float)sample_rate,hpCoeffs[i]);
            }

            lpState[0].resize(channelCount);
            lpState[1].resize(channelCount);
            hpState[0].resize(channelCount);
            hpState[1].resize(channelCount);

            Reset();
            configured=true;

            return true;
        }

        void Reset()
        {
            for(int stage=0;stage<2;stage++)
            {
                for(uint32 ch=0;ch<channelCount;ch++)
                {
                    lpState[stage][ch]={ 0.0f,0.0f,0.0f,0.0f };
                    hpState[stage][ch]={ 0.0f,0.0f,0.0f,0.0f };
                }
            }
        }

        /** 分频：input → 低通/高通输出（每通道） */
        void Process(const float *const *input,float *const *low_pass_output,float *const *high_pass_output,uint32 sample_count)
        {
            if(!configured||sample_count==0)
                return;

            for(uint32 ch=0;ch<channelCount;ch++)
            {
                const float *in=input[ch];
                float *lp_out=low_pass_output[ch];
                float *hp_out=high_pass_output[ch];

                for(uint32 i=0;i<sample_count;i++)
                {
                    const float sample=in[i];

                    // 低通路径：两级 LP biquad
                    float temp=sample;

                    for(int stage=0;stage<2;stage++)
                    {
                        BiquadState &state=lpState[stage][ch];
                        const BiquadCoefficients &c=lpCoeffs[stage];

                        float y=c.b0*temp+c.b1*state.x1+c.b2*state.x2-c.a1*state.y1-c.a2*state.y2;

                        state.x2=state.x1;
                        state.x1=temp;
                        state.y2=state.y1;
                        state.y1=y;

                        // 防 denormal
                        if(std::abs(y)<1e-10f)
                            y=0.0f;

                        temp=y;
                    }

                    lp_out[i]=temp;

                    // 高通路径：两级 HP biquad
                    temp=sample;

                    for(int stage=0;stage<2;stage++)
                    {
                        BiquadState &state=hpState[stage][ch];
                        const BiquadCoefficients &c=hpCoeffs[stage];

                        float y=c.b0*temp+c.b1*state.x1+c.b2*state.x2-c.a1*state.y1-c.a2*state.y2;

                        state.x2=state.x1;
                        state.x1=temp;
                        state.y2=state.y1;
                        state.y1=y;

                        if(std::abs(y)<1e-10f)
                            y=0.0f;

                        temp=y;
                    }

                    hp_out[i]=temp;
                }
            }
        }

    private:
        // RBJ Audio EQ Cookbook（公共领域，Robert Bristow-Johnson）
        static void ComputeLowPassCoefficients(float frequency,float sample_rate,BiquadCoefficients &out)
        {
            const float omega=2.0f*3.14159265358979323846f*frequency/sample_rate;
            const float cosOmega=std::cos(omega);
            const float sinOmega=std::sin(omega);
            const float alpha=sinOmega/(2.0f*kButterworthQ);
            const float a0=1.0f+alpha;

            out.b0=(1.0f-cosOmega)/2.0f/a0;
            out.b1=(1.0f-cosOmega)/a0;
            out.b2=(1.0f-cosOmega)/2.0f/a0;
            out.a1=-2.0f*cosOmega/a0;
            out.a2=(1.0f-alpha)/a0;
        }

        static void ComputeHighPassCoefficients(float frequency,float sample_rate,BiquadCoefficients &out)
        {
            const float omega=2.0f*3.14159265358979323846f*frequency/sample_rate;
            const float cosOmega=std::cos(omega);
            const float sinOmega=std::sin(omega);
            const float alpha=sinOmega/(2.0f*kButterworthQ);
            const float a0=1.0f+alpha;

            out.b0=(1.0f+cosOmega)/2.0f/a0;
            out.b1=-(1.0f+cosOmega)/a0;
            out.b2=(1.0f+cosOmega)/2.0f/a0;
            out.a1=-2.0f*cosOmega/a0;
            out.a2=(1.0f-alpha)/a0;
        }

        uint32 channelCount=0;
        uint32 sampleRate=0;
        float crossoverFrequency=0.0f;
        bool configured=false;

        BiquadCoefficients lpCoeffs[2];
        BiquadCoefficients hpCoeffs[2];
        std::vector<BiquadState> lpState[2];
        std::vector<BiquadState> hpState[2];
    };
}//namespace hgl::audio
