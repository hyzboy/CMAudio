#pragma once

#include<hgl/audio/Ambisonics/AmbisonicComponent.h>
#include<hgl/audio/Ambisonics/AmbisonicSpeaker.h>
#include<hgl/audio/Ambisonics/BFormat.h>
#include<vector>

namespace hgl::audio
{
    class BFormat;

    /**
    * Ambisonics 解码器（R2 移植，源自 Amplitude Audio SDK，Apache 2.0）
    *
    * BFormat 声场 → 扬声器布局馈送。内置 Mono/Stereo/5.1/7.1/Cube/Dodeca/Lebedev 预设，
    * 每扬声器使用解码系数（order weights + SN3D）。
    */
    class AmbisonicDecoder : public AmbisonicComponent
    {
    public:
        AmbisonicDecoder()=default;
        ~AmbisonicDecoder()override=default;

        /** 配置：阶数 + 3D + 扬声器预设（Custom 时需 speaker_count） */
        bool Configure(uint32 _order,bool _is3D,SpeakersPreset setUp,uint32 speaker_count=0)
        {
            if(!AmbisonicComponent::Configure(_order,_is3D))
                return false;

            SetUpSpeakers(setUp,speaker_count);
            Refresh();

            return true;
        }

        void Reset()override
        {
            for(auto &s:speakers)
                s.Reset();
        }

        void Refresh()override
        {
            for(auto &s:speakers)
                s.Refresh();

            DetectSpeakersPreset();
            LoadDecoderPreset();
        }

        /** 解码：BFormat → 多声道扬声器输出 */
        void Process(BFormat *input,uint32 samples,AmbisonicBuffer &output)
        {
            for(uint32 i=0;i<speakerCount;i++)
                speakers[i].Process(input,samples,output[i]);
        }

        SpeakersPreset GetSpeakerSetUp()const{return speakersPreset;}
        uint32 GetSpeakerCount()const{return speakerCount;}

        void SetSpeakerPosition(uint32 speaker,const SphericalPosition &position)
        {
            if(speaker<speakerCount)
                speakers[speaker].SetPosition(position);
        }

        SphericalPosition GetSpeakerPosition(uint32 speaker)const
        {
            return (speaker<speakerCount)?speakers[speaker].GetPosition():SphericalPosition();
        }

        void SetSpeakerOrderWeight(uint32 speaker,uint32 _order,float weight)
        {
            if(speaker<speakerCount)
                speakers[speaker].SetOrderWeight(_order,weight);
        }

        float GetSpeakerOrderWeight(uint32 speaker,uint32 _order)const
        {
            return (speaker<speakerCount)?speakers[speaker].GetOrderWeight(_order):0.0f;
        }

        void SetSpeakerCoefficient(uint32 speaker,uint32 channel,float coefficient)
        {
            if(speaker<speakerCount)
            {
                // 通过实体设置：直接改系数需要额外接口，用 order weight 近似场景
                // 这里提供显式覆盖（通过重新定位实现复杂，保留基础版）
                speakers[speaker].SetCoefficient(channel,coefficient);
            }
        }

        float GetSpeakerCoefficient(uint32 speaker,uint32 channel)const
        {
            return (speaker<speakerCount)?speakers[speaker].GetCoefficient(channel):0.0f;
        }

        bool IsLoaded()const{return isLoaded;}

    private:
        void SetUpSpeakers(SpeakersPreset setUp,uint32 speaker_count);

        void DetectSpeakersPreset();
        void LoadDecoderPreset();

        SpeakersPreset speakersPreset=SpeakersPreset::COUNT;
        uint32 speakerCount=0;
        std::vector<AmbisonicSpeaker> speakers;
        bool isLoaded=false;
    };
}//namespace hgl::audio
