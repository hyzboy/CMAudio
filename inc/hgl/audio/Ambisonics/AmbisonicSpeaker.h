#pragma once

#include<hgl/audio/Ambisonics/AmbisonicEntity.h>
#include<hgl/audio/Ambisonics/BFormat.h>

namespace hgl::audio
{
    class BFormat;

    /**
    * Ambisonics 扬声器（R2 移植，源自 Amplitude Audio SDK，Apache 2.0）
    *
    * 一个物理扬声器：位置→系数，将 BFormat 声场解码为该扬声器的馈送。
    */
    class AmbisonicSpeaker : public AmbisonicEntity
    {
    public:
        AmbisonicSpeaker()
        {
            Configure(1,true);
            Refresh();
        }

        ~AmbisonicSpeaker()override=default;

        bool Configure(uint32 _order,bool _is3D)override
        {
            if(!AmbisonicEntity::Configure(_order,_is3D))
                return false;

            SetOrderWeight(0,std::sqrt(2.0f));

            return true;
        }

        void Refresh()override
        {
            AmbisonicEntity::Refresh();
        }

        /** 解码：BFormat → 本扬声器单声道输出（系数加权累加） */
        void Process(BFormat *input,uint32 frame_count,AudioChannel &output)
        {
            output.clear();

            for(uint32 c=0;c<channelCount;c++)
            {
                const AudioChannel &channel=input->GetBufferChannel(c);

                AmbScalarMultiplyAccumulate(channel.begin(),output.begin(),coefficients[c],frame_count);
            }
        }
    };
}//namespace hgl::audio
