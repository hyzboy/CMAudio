#pragma once

#include<hgl/audio/Ambisonics/AmbisonicComponent.h>
#include<cmath>
#include<cstring>
#include<vector>

namespace hgl::audio
{
    /**
    * Ambisonics 实体基类（位置→球谐系数，R2 移植，源自 Amplitude Audio SDK，Apache 2.0）
    *
    * 使用 ACN 通道排序 + SN3D 归一化（AmbiX 格式）。
    * 系数 = 方向球谐函数值 × 阶权重 × 增益
    */
    class AmbisonicEntity : public AmbisonicComponent
    {
    public:
        AmbisonicEntity()
            : position(0.0f,0.0f,1.0f),gain(1.0f)
        {}

        ~AmbisonicEntity()override=default;

        bool Configure(uint32 _order,bool _is3D)override
        {
            if(!AmbisonicComponent::Configure(_order,_is3D))
                return false;

            coefficients.resize(channelCount,0.0f);
            orderWeights.resize(order+1,1.0f);

            return true;
        }

        void Reset()override
        {
            std::fill(coefficients.begin(),coefficients.end(),0.0f);
        }

        void Refresh()override
        {
            const float cosAzim=std::cos(position.GetAzimuth());
            const float sinAzim=std::sin(position.GetAzimuth());
            const float cosElev=std::cos(position.GetElevation());
            const float sinElev=std::sin(position.GetElevation());

            const float cos2Azim=std::cos(2.0f*position.GetAzimuth());
            const float sin2Azim=std::sin(2.0f*position.GetAzimuth());
            const float sin2Elev=std::sin(2.0f*position.GetElevation());

            constexpr float sqrt32=0.8660254037844386f;      // sqrt(3)/2
            constexpr float sqrt58=0.7905694150420949f;      // sqrt(5/8)
            constexpr float sqrt152=1.9364916731037085f;     // sqrt(15)/2
            constexpr float sqrt38=0.6123724356957945f;      // sqrt(3/8)

            if(is3D)
            {
                // W
                coefficients[(int)BFormatChannel::W]=1.0f*orderWeights[0];

                if(order>=1)
                {
                    coefficients[(int)BFormatChannel::Y]=sinAzim*cosElev*orderWeights[1];
                    coefficients[(int)BFormatChannel::Z]=sinElev*orderWeights[1];
                    coefficients[(int)BFormatChannel::X]=cosAzim*cosElev*orderWeights[1];
                }

                if(order>=2)
                {
                    coefficients[(int)BFormatChannel::V]=sqrt32*(sin2Azim*AMB_SQUARED(cosElev))*orderWeights[2];
                    coefficients[(int)BFormatChannel::T]=sqrt32*(sinAzim*sin2Elev)*orderWeights[2];
                    coefficients[(int)BFormatChannel::R]=(1.5f*AMB_SQUARED(sinElev)-0.5f)*orderWeights[2];
                    coefficients[(int)BFormatChannel::S]=sqrt32*(cosAzim*sin2Elev)*orderWeights[2];
                    coefficients[(int)BFormatChannel::U]=sqrt32*(cos2Azim*AMB_SQUARED(cosElev))*orderWeights[2];
                }

                if(order>=3)
                {
                    coefficients[(int)BFormatChannel::Q]=sqrt58*(std::sin(3.0f*position.GetAzimuth())*AMB_CUBED(cosElev))*orderWeights[3];
                    coefficients[(int)BFormatChannel::O]=sqrt152*(sin2Azim*sinElev*AMB_SQUARED(cosElev))*orderWeights[3];
                    coefficients[(int)BFormatChannel::M]=sqrt38*(sinAzim*cosElev*(5.0f*AMB_SQUARED(sinElev)-1.0f))*orderWeights[3];
                    coefficients[(int)BFormatChannel::K]=sinElev*(5.0f*AMB_SQUARED(sinElev)-3.0f)*0.5f*orderWeights[3];
                    coefficients[(int)BFormatChannel::L]=sqrt38*(cosAzim*cosElev*(5.0f*AMB_SQUARED(sinElev)-1.0f))*orderWeights[3];
                    coefficients[(int)BFormatChannel::N]=sqrt152*(cos2Azim*sinElev*AMB_SQUARED(cosElev))*orderWeights[3];
                    coefficients[(int)BFormatChannel::P]=sqrt58*(std::cos(3.0f*position.GetAzimuth())*AMB_CUBED(cosElev))*orderWeights[3];
                }
            }
            else
            {
                coefficients[0]=1.0f*orderWeights[0];

                if(order>=1)
                {
                    coefficients[1]=cosAzim*cosElev*orderWeights[1];
                    coefficients[2]=sinAzim*cosElev*orderWeights[1];
                }

                if(order>=2)
                {
                    coefficients[3]=cos2Azim*AMB_SQUARED(cosElev)*orderWeights[2];
                    coefficients[4]=sin2Azim*AMB_SQUARED(cosElev)*orderWeights[2];
                }

                if(order>=3)
                {
                    coefficients[5]=std::cos(3.0f*position.GetAzimuth())*AMB_CUBED(cosElev)*orderWeights[3];
                    coefficients[6]=std::sin(3.0f*position.GetAzimuth())*AMB_CUBED(cosElev)*orderWeights[3];
                }
            }

            AmbScalarMultiply(coefficients.data(),coefficients.data(),gain,channelCount);
        }

        void SetPosition(const SphericalPosition &_position)
        {
            position=_position;
            Refresh();
        }

        void SetPosition(const SphericalPosition &_position,double duration)
        {
            // 简化：无插值时直接设置（R2 基础版）
            position=_position;
            Refresh();
        }

        void SetGain(float _gain)
        {
            gain=_gain;
            Refresh();
        }

        float GetCoefficient(uint32 channel)const{return coefficients[channel];}
        const std::vector<float> &GetCoefficients()const{return coefficients;}

        void SetCoefficient(uint32 channel,float coefficient)
        {
            if(channel<coefficients.size())
                coefficients[channel]=coefficient;
        }

        void SetWeight(float _gain)
        {
            gain=_gain;
            Refresh();
        }

        void SetOrderWeight(uint32 _order,float weight)
        {
            if(_order<orderWeights.size())
                orderWeights[_order]=weight;

            Refresh();
        }

        float GetOrderWeight(uint32 _order)const
        {
            return (_order<orderWeights.size())?orderWeights[_order]:0.0f;
        }

        const SphericalPosition &GetPosition()const{return position;}

    protected:
        SphericalPosition position;
        float gain=1.0f;
        std::vector<float> coefficients;
        std::vector<float> orderWeights;
    };
}//namespace hgl::audio
