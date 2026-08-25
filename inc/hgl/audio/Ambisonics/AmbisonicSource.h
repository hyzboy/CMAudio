#pragma once

#include<hgl/audio/Ambisonics/AmbisonicEntity.h>
#include<hgl/audio/Ambisonics/BFormat.h>

namespace hgl::audio
{
    /**
    * Ambisonics 单声源编码器（R2 移植，源自 Amplitude Audio SDK，Apache 2.0）
    *
    * 将单声道输入按位置球谐系数编码进 BFormat 声场。
    */
    class AmbisonicSource : public AmbisonicEntity
    {
    public:
        AmbisonicSource()=default;
        ~AmbisonicSource()override=default;

        bool Configure(uint32 _order,bool _is3D)override
        {
            if(!AmbisonicEntity::Configure(_order,_is3D))
                return false;

            oldCoefficients.resize(channelCount,0.0f);
            interpolationDuration=0.0;

            return true;
        }

        void Refresh()override
        {
            AmbisonicEntity::Refresh();
        }

        void SetPosition(const SphericalPosition &_position)
        {
            AmbisonicEntity::SetPosition(_position);
        }

        /** 带插值时长（简化：立即生效，插值 R2 不做） */
        void SetPositionInterpolated(const SphericalPosition &_position,double duration)
        {
            AmbisonicEntity::SetPosition(_position,duration);
        }

        /** 编码单声道输入到 BFormat（累加模式，offset/gain 可选） */
        void ProcessAccumulate(const AudioChannel &input,uint32 samples,BFormat *output,uint32 offset=0,float _gain=1.0f)const
        {
            if(!output)
                return;

            const float *src=input.begin();

            for(uint32 c=0;c<channelCount;c++)
            {
                AudioChannel &dst=output->GetBufferChannel(c);

                const float coeff=coefficients[c]*_gain;

                for(uint32 i=0;i<samples;i++)
                    dst[offset+i]+=src[i]*coeff;
            }
        }

        /** 编码单声道输入到 BFormat（覆盖模式） */
        void Process(const AudioChannel &input,uint32 samples,BFormat *output)const
        {
            if(!output)
                return;

            const float *src=input.begin();

            for(uint32 c=0;c<channelCount;c++)
            {
                AudioChannel &dst=output->GetBufferChannel(c);

                const float coeff=coefficients[c];

                for(uint32 i=0;i<samples;i++)
                    dst[i]=src[i]*coeff;
            }
        }

    private:
        std::vector<float> oldCoefficients;
        double interpolationDuration=0.0;
    };
}//namespace hgl::audio
