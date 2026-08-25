#pragma once

#include<hgl/audio/Ambisonics/AmbisonicComponent.h>
#include<hgl/audio/Ambisonics/BFormat.h>
#include<hgl/audio/Ambisonics/LinkwitzRileyFilter.h>
#include<vector>
#include<algorithm>
#include<cmath>

namespace hgl::audio
{
    /**
    * Ambisonics 心理声学校准滤波（R2 移植，源自 Amplitude Audio SDK，Apache 2.0）
    *
    * 高阶 Ambisonics 的双耳化前置校准：按阶数应用 max-rE 增益的高频 shelving
    * （低通/高通分裂后按阶加权重组），补偿头部遮挡效应。
    */
    class AmbisonicShelfFilter : public AmbisonicComponent
    {
        static constexpr float kHeadRadius=0.09f;               // 平均头半径（米，Bertet et al. 2013）
        static constexpr float kMaxReAngleDegrees=137.9f;       // max-rE 增益计算角（Zotter & Frank）

    public:
        AmbisonicShelfFilter()=default;
        ~AmbisonicShelfFilter()override=default;

        bool Configure(uint32 _order,bool _is3D,uint32 max_block_size,uint32 sample_rate,float speed_of_sound=343.0f)
        {
            if(_order<1||_order>3)
                return false;

            if(max_block_size==0||sample_rate==0)
                return false;

            if(!AmbisonicComponent::Configure(_order,_is3D))
                return false;

            maxBlockSize=max_block_size;
            sampleRate=sample_rate;
            speedOfSound=speed_of_sound;

            const float crossover_freq=ComputeCrossoverFrequency(_order,speed_of_sound);

            if(!crossoverFilter.Configure(channelCount,sample_rate,crossover_freq))
                return false;

            if(!lowPassBuffer.Configure(_order,_is3D,max_block_size))
                return false;

            tempInputBuffer=AmbisonicBuffer(max_block_size,channelCount);

            inputPtrs.resize(channelCount);
            lpPtrs.resize(channelCount);
            hpPtrs.resize(channelCount);
            channelOrders.resize(channelCount);

            for(uint32 ch=0;ch<channelCount;ch++)
                channelOrders[ch]=ChannelToOrder(ch);

            highFreqGains=ComputeMaxReGains(_order,_is3D);

            configured=true;

            return true;
        }

        void Reset()override
        {
            crossoverFilter.Reset();

            lowPassBuffer.GetBuffer()->Clear();
        }

        void Refresh()override{}

        std::vector<float> GetMaxReGains()const{return highFreqGains;}

        void SetHighFrequencyGains(const std::vector<float> &gains)
        {
            if(gains.size()==order+1)
                highFreqGains=gains;
        }

        float GetCrossoverFrequency()const
        {
            return ComputeCrossoverFrequency(order,speedOfSound);
        }

        void Process(BFormat *buffer,uint32 sample_count)
        {
            if(!configured||buffer==nullptr||sample_count==0)
                return;

            if(sample_count>maxBlockSize)
                return;

            for(uint32 ch=0;ch<channelCount;ch++)
            {
                lpPtrs[ch]=lowPassBuffer.GetBufferChannel(ch).begin();
                hpPtrs[ch]=buffer->GetBufferChannel(ch).begin();
            }

            for(uint32 ch=0;ch<channelCount;ch++)
            {
                const float *src=buffer->GetBufferChannel(ch).begin();

                std::copy(src,src+sample_count,tempInputBuffer[ch].begin());
                inputPtrs[ch]=tempInputBuffer[ch].begin();
            }

            crossoverFilter.Process(inputPtrs.data(),lpPtrs.data(),hpPtrs.data(),sample_count);

            for(uint32 ch=0;ch<channelCount;ch++)
            {
                const float gain=highFreqGains[channelOrders[ch]];

                float *hp_data=hpPtrs[ch];
                const float *lp_data=lpPtrs[ch];

                for(uint32 i=0;i<sample_count;i++)
                    hp_data[i]=gain*hp_data[i]+lp_data[i];
            }
        }

    private:
        static float Legendre(uint32 n,float x)
        {
            if(n==0)
                return 1.0f;
            if(n==1)
                return x;

            float p_prev2=1.0f;
            float p_prev1=x;
            float p_cur=0.0f;

            for(uint32 i=1;i<n;i++)
            {
                p_cur=((2.0f*static_cast<float>(i)+1.0f)*x*p_prev1-static_cast<float>(i)*p_prev2)/static_cast<float>(i+1);
                p_prev2=p_prev1;
                p_prev1=p_cur;
            }

            return p_cur;
        }

        static uint32 ChannelToOrder(uint32 channel)
        {
            return (uint32)std::floor(std::sqrt((float)channel));
        }

        static float ComputeCrossoverFrequency(uint32 _order,float _speed_of_sound)
        {
            if(_order==0||_speed_of_sound<=0.0f)
                return 0.0f;

            const float M=static_cast<float>(_order);
            const float numerator=_speed_of_sound*M;
            const float denominator=4.0f*kHeadRadius*(M+1.0f)*std::sin(3.14159265358979323846f/(2.0f*M+2.0f));

            return numerator/denominator;
        }

        static std::vector<float> ComputeMaxReGains(uint32 _order,bool _is3D)
        {
            std::vector<float> gains(_order+1);

            const float N=static_cast<float>(_order);

            if(_is3D)
            {
                // 3D max-rE：Legendre 多项式（Zotter & Frank）
                const float theta_degrees=kMaxReAngleDegrees/(N+1.51f);
                const float theta_radians=theta_degrees*3.14159265358979323846f/180.0f;
                const float cos_theta=std::cos(theta_radians);

                for(uint32 n=0;n<=_order;n++)
                    gains[n]=Legendre(n,cos_theta);

                // 能量补偿：(N+1)/sqrt(sum(P_n^2*(2n+1)))
                float sum_squared=0.0f;

                for(uint32 n=0;n<=_order;n++)
                {
                    const float n_float=static_cast<float>(n);

                    sum_squared+=gains[n]*gains[n]*(2.0f*n_float+1.0f);
                }

                constexpr float threshold=1e-10f;
                const float energy_comp=(sum_squared>threshold)?(N+1.0f)/std::sqrt(sum_squared):1.0f;

                for(uint32 n=0;n<=_order;n++)
                    gains[n]*=energy_comp;
            }
            else
            {
                // 2D max-rE：余弦公式（Daniel 2000）
                const float energy_comp=std::sqrt((2.0f*N+1.0f)/(N+1.0f));

                for(uint32 n=0;n<=_order;n++)
                {
                    const float n_float=static_cast<float>(n);

                    gains[n]=std::cos(n_float*3.14159265358979323846f/(2.0f*N+2.0f))*energy_comp;
                }
            }

            return gains;
        }

        uint32 maxBlockSize=0;
        uint32 sampleRate=0;
        float speedOfSound=343.0f;
        bool configured=false;

        LinkwitzRileyFilter crossoverFilter;
        BFormat lowPassBuffer;

        AmbisonicBuffer tempInputBuffer;
        std::vector<float*> inputPtrs;
        std::vector<float*> lpPtrs;
        std::vector<float*> hpPtrs;
        std::vector<uint32> channelOrders;
        std::vector<float> highFreqGains;
    };
}//namespace hgl::audio
