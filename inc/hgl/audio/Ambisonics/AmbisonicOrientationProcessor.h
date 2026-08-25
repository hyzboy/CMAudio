#pragma once

#include<hgl/audio/Ambisonics/AmbisonicComponent.h>
#include<hgl/audio/Ambisonics/BFormat.h>
#include<cmath>
#include<vector>

namespace hgl::audio
{
    /**
    * Ambisonics 声场旋转器（R2 移植，源自 Amplitude Audio SDK，Apache 2.0）
    *
    * 用欧拉角（alpha/beta/gamma，ZYX 序）旋转 BFormat 声场。
    * 每阶球谐系数按旋转矩阵变换（1/2/3 阶）。
    */
    class AmbisonicOrientationProcessor : public AmbisonicComponent
    {
    public:
        AmbisonicOrientationProcessor()=default;
        ~AmbisonicOrientationProcessor()override=default;

        bool Configure(uint32 _order,bool _is3D)override
        {
            if(!AmbisonicComponent::Configure(_order,_is3D))
                return false;

            tempSamples.resize((int)BFormatChannel::COUNT,0.0f);

            return true;
        }

        void Refresh()override
        {
            cosAlpha=std::cos(orientation.GetAlpha());
            sinAlpha=std::sin(orientation.GetAlpha());
            cosBeta=std::cos(orientation.GetBeta());
            sinBeta=std::sin(orientation.GetBeta());
            cosGamma=std::cos(orientation.GetGamma());
            sinGamma=std::sin(orientation.GetGamma());

            cos2Alpha=std::cos(2.0f*orientation.GetAlpha());
            sin2Alpha=std::sin(2.0f*orientation.GetAlpha());
            cos2Beta=std::cos(2.0f*orientation.GetBeta());
            sin2Beta=std::sin(2.0f*orientation.GetBeta());
            cos2Gamma=std::cos(2.0f*orientation.GetGamma());
            sin2Gamma=std::sin(2.0f*orientation.GetGamma());

            cos3Alpha=std::cos(3.0f*orientation.GetAlpha());
            sin3Alpha=std::sin(3.0f*orientation.GetAlpha());
            cos3Beta=std::cos(3.0f*orientation.GetBeta());
            sin3Beta=std::sin(3.0f*orientation.GetBeta());
            cos3Gamma=std::cos(3.0f*orientation.GetGamma());
            sin3Gamma=std::sin(3.0f*orientation.GetGamma());
        }

        void Reset()override{}

        void SetOrientation(float alpha,float beta,float gamma)
        {
            orientation.SetOrientation(alpha,beta,gamma);
            Refresh();
        }

        const Orientation &GetOrientation()const{return orientation;}

        void Process(BFormat *input,uint32 samples)
        {
            if(!is3D)
                return; // 3D input expected

            if(order>=1)
                ProcessOrder1(input,samples);
            if(order>=2)
                ProcessOrder2(input,samples);
            if(order>=3)
                ProcessOrder3(input,samples);
        }

    private:
        static constexpr float sqrt3=1.7320508075688772f;       // sqrt(3)
        static constexpr float sqrt3_2=1.224744871391589f;      // sqrt(3/2)
        static constexpr float sqrt5_2=1.5811388300841898f;      // sqrt(5/2)
        static constexpr float sqrt15=3.872983346207417f;        // sqrt(15)

        void ProcessOrder1(BFormat *input,uint32 samples)
        {
            AudioChannel &xChannel=input->GetBufferChannel((int)BFormatChannel::X);
            AudioChannel &yChannel=input->GetBufferChannel((int)BFormatChannel::Y);
            AudioChannel &zChannel=input->GetBufferChannel((int)BFormatChannel::Z);

            for(uint32 i=0;i<samples;i++)
            {
                // Alpha rotation (Z)
                tempSamples[(int)BFormatChannel::X]=xChannel[i]*cosAlpha+yChannel[i]*sinAlpha;
                tempSamples[(int)BFormatChannel::Y]=yChannel[i]*cosAlpha-xChannel[i]*sinAlpha;
                tempSamples[(int)BFormatChannel::Z]=zChannel[i];

                // Beta rotation (Y)
                xChannel[i]=tempSamples[(int)BFormatChannel::X]*cosBeta-tempSamples[(int)BFormatChannel::Z]*sinBeta;
                yChannel[i]=tempSamples[(int)BFormatChannel::Y];
                zChannel[i]=tempSamples[(int)BFormatChannel::Z]*cosBeta+tempSamples[(int)BFormatChannel::X]*sinBeta;

                // Gamma rotation (X)
                tempSamples[(int)BFormatChannel::X]=xChannel[i]*cosGamma+yChannel[i]*sinGamma;
                tempSamples[(int)BFormatChannel::Y]=yChannel[i]*cosGamma-xChannel[i]*sinGamma;
                tempSamples[(int)BFormatChannel::Z]=zChannel[i];

                xChannel[i]=tempSamples[(int)BFormatChannel::X];
                yChannel[i]=tempSamples[(int)BFormatChannel::Y];
                zChannel[i]=tempSamples[(int)BFormatChannel::Z];
            }
        }

        void ProcessOrder2(BFormat *input,uint32 samples)
        {
            AudioChannel &rChannel=input->GetBufferChannel((int)BFormatChannel::R);
            AudioChannel &sChannel=input->GetBufferChannel((int)BFormatChannel::S);
            AudioChannel &tChannel=input->GetBufferChannel((int)BFormatChannel::T);
            AudioChannel &uChannel=input->GetBufferChannel((int)BFormatChannel::U);
            AudioChannel &vChannel=input->GetBufferChannel((int)BFormatChannel::V);

            for(uint32 i=0;i<samples;i++)
            {
                // Alpha rotation (Z)
                tempSamples[(int)BFormatChannel::R]=rChannel[i];
                tempSamples[(int)BFormatChannel::S]=sChannel[i]*cosAlpha+tChannel[i]*sinAlpha;
                tempSamples[(int)BFormatChannel::T]=tChannel[i]*cosAlpha-sChannel[i]*sinAlpha;
                tempSamples[(int)BFormatChannel::U]=uChannel[i]*cos2Alpha+vChannel[i]*sin2Alpha;
                tempSamples[(int)BFormatChannel::V]=vChannel[i]*cos2Alpha-uChannel[i]*sin2Alpha;

                // Beta rotation (Y)
                rChannel[i]=tempSamples[(int)BFormatChannel::R]*(0.75f*cosBeta+0.25f)
                           +tempSamples[(int)BFormatChannel::U]*(0.5f*sqrt3*AMB_SQUARED(sinBeta))
                           +tempSamples[(int)BFormatChannel::S]*(sqrt3*sinBeta*cosBeta);
                sChannel[i]=tempSamples[(int)BFormatChannel::S]*cos2Beta
                           -tempSamples[(int)BFormatChannel::R]*cosBeta*sinBeta*sqrt3
                           +tempSamples[(int)BFormatChannel::U]*cosBeta*sinBeta;
                tChannel[i]=tempSamples[(int)BFormatChannel::V]*sinBeta-tempSamples[(int)BFormatChannel::T]*cosBeta;
                uChannel[i]=tempSamples[(int)BFormatChannel::U]*(0.25f*cos2Beta+0.75f)
                           -tempSamples[(int)BFormatChannel::S]*cosBeta*sinBeta
                           +tempSamples[(int)BFormatChannel::R]*(0.5f*sqrt3*AMB_SQUARED(sinBeta));
                vChannel[i]=tempSamples[(int)BFormatChannel::V]*cosBeta-tempSamples[(int)BFormatChannel::T]*sinBeta;

                // Gamma rotation (X)
                tempSamples[(int)BFormatChannel::R]=rChannel[i];
                tempSamples[(int)BFormatChannel::S]=sChannel[i]*cosGamma+tChannel[i]*sinGamma;
                tempSamples[(int)BFormatChannel::T]=tChannel[i]*cosGamma-sChannel[i]*sinGamma;
                tempSamples[(int)BFormatChannel::U]=uChannel[i]*cos2Gamma+vChannel[i]*sin2Gamma;
                tempSamples[(int)BFormatChannel::V]=vChannel[i]*cos2Gamma-uChannel[i]*sin2Gamma;

                rChannel[i]=tempSamples[(int)BFormatChannel::R];
                sChannel[i]=tempSamples[(int)BFormatChannel::S];
                tChannel[i]=tempSamples[(int)BFormatChannel::T];
                uChannel[i]=tempSamples[(int)BFormatChannel::U];
                vChannel[i]=tempSamples[(int)BFormatChannel::V];
            }
        }

        void ProcessOrder3(BFormat *input,uint32 samples)
        {
            AudioChannel &kChannel=input->GetBufferChannel((int)BFormatChannel::K);
            AudioChannel &lChannel=input->GetBufferChannel((int)BFormatChannel::L);
            AudioChannel &mChannel=input->GetBufferChannel((int)BFormatChannel::M);
            AudioChannel &nChannel=input->GetBufferChannel((int)BFormatChannel::N);
            AudioChannel &oChannel=input->GetBufferChannel((int)BFormatChannel::O);
            AudioChannel &pChannel=input->GetBufferChannel((int)BFormatChannel::P);
            AudioChannel &qChannel=input->GetBufferChannel((int)BFormatChannel::Q);

            for(uint32 i=0;i<samples;i++)
            {
                // Alpha rotation (Z)
                tempSamples[(int)BFormatChannel::K]=kChannel[i];
                tempSamples[(int)BFormatChannel::L]=lChannel[i]*cosAlpha+mChannel[i]*sinAlpha;
                tempSamples[(int)BFormatChannel::M]=mChannel[i]*cosAlpha-lChannel[i]*sinAlpha;
                tempSamples[(int)BFormatChannel::N]=nChannel[i]*cos2Alpha+oChannel[i]*sin2Alpha;
                tempSamples[(int)BFormatChannel::O]=oChannel[i]*cos2Alpha-nChannel[i]*sin2Alpha;
                tempSamples[(int)BFormatChannel::P]=pChannel[i]*cos3Alpha+qChannel[i]*sin3Alpha;
                tempSamples[(int)BFormatChannel::Q]=qChannel[i]*cos3Alpha-pChannel[i]*sin3Alpha;

                // Beta rotation (Y)
                qChannel[i]=0.125f*tempSamples[(int)BFormatChannel::Q]*(5.0f+3.0f*cos2Beta)
                            -sqrt3_2*tempSamples[(int)BFormatChannel::O]*cosBeta*sinBeta
                            +0.25f*sqrt15*tempSamples[(int)BFormatChannel::M]*powf(sinBeta,2.0f);
                oChannel[i]=tempSamples[(int)BFormatChannel::O]*cos2Beta
                            -sqrt5_2*tempSamples[(int)BFormatChannel::M]*cosBeta*sinBeta
                            +sqrt3_2*tempSamples[(int)BFormatChannel::Q]*cosBeta*sinBeta;
                mChannel[i]=0.125f*tempSamples[(int)BFormatChannel::M]*(3.0f+5.0f*cos2Beta)
                            -sqrt5_2*tempSamples[(int)BFormatChannel::O]*cosBeta*sinBeta
                            +0.25f*sqrt15*tempSamples[(int)BFormatChannel::Q]*powf(sinBeta,2.0f);
                kChannel[i]=0.25f*tempSamples[(int)BFormatChannel::K]*cosBeta*(-1.0f+15.0f*cos2Beta)
                            +0.5f*sqrt15*tempSamples[(int)BFormatChannel::N]*cosBeta*powf(sinBeta,2.0f)
                            +0.5f*sqrt5_2*tempSamples[(int)BFormatChannel::P]*powf(sinBeta,3.0f)
                            +0.125f*sqrt3_2*tempSamples[(int)BFormatChannel::L]*(sinBeta+5.0f*sin3Beta);
                lChannel[i]=0.0625f*tempSamples[(int)BFormatChannel::L]*(cosBeta+15.0f*cos3Beta)
                            +0.25f*sqrt5_2*tempSamples[(int)BFormatChannel::N]*(1.0f+3.0f*cos2Beta)*sinBeta
                            +0.25f*sqrt15*tempSamples[(int)BFormatChannel::P]*cosBeta*powf(sinBeta,2.0f)
                            -0.125f*sqrt3_2*tempSamples[(int)BFormatChannel::K]*(sinBeta+5.0f*sin3Beta);
                nChannel[i]=0.125f*tempSamples[(int)BFormatChannel::N]*(5.0f*cosBeta+3.0f*cos3Beta)
                            +0.25f*sqrt3_2*tempSamples[(int)BFormatChannel::P]*(3.0f+cos2Beta)*sinBeta
                            +0.5f*sqrt15*tempSamples[(int)BFormatChannel::K]*cosBeta*powf(sinBeta,2.0f)
                            +0.125f*sqrt5_2*tempSamples[(int)BFormatChannel::L]*(sinBeta-3.0f*sin3Beta);
                pChannel[i]=0.0625f*tempSamples[(int)BFormatChannel::P]*(15.0f*cosBeta+cos3Beta)
                            -0.25f*sqrt3_2*tempSamples[(int)BFormatChannel::N]*(3.0f+cos2Beta)*sinBeta
                            +0.25f*sqrt15*tempSamples[(int)BFormatChannel::L]*cosBeta*powf(sinBeta,2.0f)
                            -0.5f*sqrt5_2*tempSamples[(int)BFormatChannel::K]*powf(sinBeta,3.0f);

                // Gamma rotation (X)
                tempSamples[(int)BFormatChannel::K]=kChannel[i];
                tempSamples[(int)BFormatChannel::L]=lChannel[i]*cosGamma+mChannel[i]*sinGamma;
                tempSamples[(int)BFormatChannel::M]=mChannel[i]*cosGamma-lChannel[i]*sinGamma;
                tempSamples[(int)BFormatChannel::N]=nChannel[i]*cos2Gamma+oChannel[i]*sin2Gamma;
                tempSamples[(int)BFormatChannel::O]=oChannel[i]*cos2Gamma-nChannel[i]*sin2Gamma;
                tempSamples[(int)BFormatChannel::P]=pChannel[i]*cos3Gamma+qChannel[i]*sin3Gamma;
                tempSamples[(int)BFormatChannel::Q]=qChannel[i]*cos3Gamma-pChannel[i]*sin3Gamma;

                kChannel[i]=tempSamples[(int)BFormatChannel::K];
                lChannel[i]=tempSamples[(int)BFormatChannel::L];
                mChannel[i]=tempSamples[(int)BFormatChannel::M];
                nChannel[i]=tempSamples[(int)BFormatChannel::N];
                oChannel[i]=tempSamples[(int)BFormatChannel::O];
                pChannel[i]=tempSamples[(int)BFormatChannel::P];
                qChannel[i]=tempSamples[(int)BFormatChannel::Q];
            }
        }

        Orientation orientation;
        std::vector<float> tempSamples;

        float cosAlpha=0,sinAlpha=0,cosBeta=0,sinBeta=0,cosGamma=0,sinGamma=0;
        float cos2Alpha=0,sin2Alpha=0,cos2Beta=0,sin2Beta=0,cos2Gamma=0,sin2Gamma=0;
        float cos3Alpha=0,sin3Alpha=0,cos3Beta=0,sin3Beta=0,cos3Gamma=0,sin3Gamma=0;
    };
}//namespace hgl::audio
