#pragma once

#include<hgl/audio/Ambisonics/AmbisonicComponent.h>
#include<hgl/audio/Ambisonics/AmbisonicDecoder.h>
#include<hgl/audio/Ambisonics/AmbisonicShelfFilter.h>
#include<hgl/audio/Ambisonics/AmbisonicSource.h>
#include<hgl/audio/Ambisonics/BFormat.h>
#include<hgl/audio/Convolver.h>
#include<hgl/audio/HRTF/HRIRSphere.h>
#include<array>
#include<vector>

namespace hgl::audio
{
    /**
    * Ambisonics 双耳化器（R2 移植，源自 Amplitude Audio SDK，Apache 2.0）
    *
    * BFormat 声场 → 左右耳双声道：
    * 1. 心理声学校准（shelf filter）
    * 2. 每个 B-Format 通道的 HRIR 由扬声器网格加权累积（解码系数 × 阶数缩放）
    * 3. 每通道卷积（左/右 HRIR）后累加输出
    */
    class AmbisonicBinauralizer : public AmbisonicComponent
    {
        static constexpr uint32 kInterpolationBlockSize=128;

    public:
        AmbisonicBinauralizer()=default;
        ~AmbisonicBinauralizer()override=default;

        bool Configure(uint32 _order,bool _is3D,uint32 max_block_size,uint32 sample_rate,const HRIRSphere *hrir_sphere)
        {
            if(hrir_sphere==nullptr)
                return false;

            if(!AmbisonicComponent::Configure(_order,_is3D))
                return false;

            if(!shelfFilter.Configure(_order,_is3D,max_block_size,sample_rate))
                return false;

            hrir=hrir_sphere;
            const uint32 hrir_length=hrir->GetIRLength();

            SetUpSpeakers();
            const uint32 n_speakers=decoder.GetSpeakerCount();

            // 左右耳累积 HRIR（每 B-Format 通道一组）
            accumulatedHRIR[0]=AmbisonicBuffer(hrir_length,channelCount);
            accumulatedHRIR[1]=AmbisonicBuffer(hrir_length,channelCount);

            // 为每个扬声器采样 HRIR 并按解码系数加权累积
            {
                AudioChannel hrir_buffer[2];
                hrir_buffer[0].resize(hrir_length,0.0f);
                hrir_buffer[1].resize(hrir_length,0.0f);

                for(uint32 c=0;c<channelCount;c++)
                {
                    AudioChannel &left_channel=accumulatedHRIR[0][c];
                    AudioChannel &right_channel=accumulatedHRIR[1][c];

                    for(uint32 i=0;i<n_speakers;i++)
                    {
                        const SphericalPosition position=decoder.GetSpeakerPosition(i);

                        hrir_buffer[0].clear();
                        hrir_buffer[1].clear();

                        hrir->Sample(position.ToCartesian(),hrir_buffer[0].begin(),hrir_buffer[1].begin());

                        // 系数缩放：SN3D 归一化的 (2*order+1) 补偿
                        const float coefficient=
                            decoder.GetSpeakerCoefficient(i,c)*(2.0f*std::floor(std::sqrt((float)c))+1.0f);

                        AmbScalarMultiplyAccumulate(hrir_buffer[0].begin(),left_channel.begin(),coefficient,hrir_length);
                        AmbScalarMultiplyAccumulate(hrir_buffer[1].begin(),right_channel.begin(),coefficient,hrir_length);
                    }
                }
            }

            // 归一化：编码 90° 方位声源求最大系数
            float max_val=0.0f;
            {
                AmbisonicSource source;
                source.Configure(_order,true);

                const SphericalPosition position90(90.0f*Amb_DegToRad,0.0f,5.0f);
                source.SetPosition(position90);

                AudioChannel right_ear90;
                right_ear90.resize(hrir_length,0.0f);

                for(uint32 c=0;c<channelCount;c++)
                {
                    const AudioChannel &accumulated_channel=accumulatedHRIR[0][c];

                    AmbScalarMultiplyAccumulate(accumulated_channel.begin(),right_ear90.begin(),source.GetCoefficient(c),hrir_length);
                }

                for(uint32 i=0;i<hrir_length;i++)
                {
                    const float val=std::fabs(right_ear90[i]);

                    if(val>max_val)
                        max_val=val;
                }
            }

            // 归一化到预定值
            const float scaler=(max_val>1e-12f)?0.35f/max_val:0.0f;

            accumulatedHRIR[0]*=scaler;
            accumulatedHRIR[1]*=scaler;

            // 每通道初始化卷积器（固定数组，3 阶 3D 最大 16 通道）
            for(uint32 c=0;c<channelCount;c++)
            {
                convL[c].Init(hrir_length,accumulatedHRIR[0][c].begin(),hrir_length);
                convR[c].Init(hrir_length,accumulatedHRIR[1][c].begin(),hrir_length);
            }

            return true;
        }

        void Reset()override
        {
            shelfFilter.Reset();
        }

        void Refresh()override{}

        /** 双耳化：BFormat → 双声道输出（左右） */
        void Process(BFormat *input,uint32 samples,AmbisonicBuffer &output)
        {
            shelfFilter.Process(input,samples);

            AmbisonicBuffer scratch(samples,2);

            AudioChannel &scratch_l=scratch[0];
            AudioChannel &scratch_r=scratch[1];
            AudioChannel &output_l=output[0];
            AudioChannel &output_r=output[1];

            output_l.clear();
            output_r.clear();

            for(uint32 c=0;c<channelCount;c++)
            {
                const AudioChannel &input_channel=input->GetBufferChannel(c);

                convL[c].Process(input_channel.begin(),scratch_l.begin(),samples);
                convR[c].Process(input_channel.begin(),scratch_r.begin(),samples);

                output_l+=scratch_l;
                output_r+=scratch_r;
            }
        }

    private:
        void SetUpSpeakers()
        {
            SpeakersPreset preset;

            if(order<=1)
                preset=SpeakersPreset::CubePoints;
            else if(order==2)
                preset=SpeakersPreset::DodecahedronFaces;
            else
                preset=SpeakersPreset::LebedevGridOrder26;

            decoder.Configure(order,is3D,preset);
        }

        const HRIRSphere *hrir=nullptr;

        AmbisonicShelfFilter shelfFilter;
        AmbisonicDecoder decoder;

        AmbisonicBuffer accumulatedHRIR[2];     // [左/右] × 通道
        // Convolver 不可拷贝/移动，用固定数组（3 阶 3D 最大 16 通道）
        std::array<Convolver,16> convL;
        std::array<Convolver,16> convR;
    };
}//namespace hgl::audio
