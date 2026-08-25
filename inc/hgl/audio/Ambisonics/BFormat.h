#pragma once

#include<hgl/audio/Ambisonics/AmbisonicComponent.h>

namespace hgl::audio
{
    class AmbisonicSource;
    class AmbisonicOrientationProcessor;

    /**
    * B-Format 声场缓冲（R2 移植，源自 Amplitude Audio SDK，Apache 2.0）
    */
    class BFormat final : public AmbisonicComponent
    {
        friend class AmbisonicSource;
        friend class AmbisonicOrientationProcessor;

    public:
        BFormat()=default;
        ~BFormat()override=default;

        void Reset()override{Refresh();}
        void Refresh()override{}

        uint32 GetSampleCount()const{return buffer.GetFrameCount();}

        /** 配置：阶数 + 3D + 采样数 */
        bool Configure(uint32 _order,bool _is3D,uint32 sample_count)
        {
            if(!AmbisonicComponent::Configure(_order,_is3D))
                return false;

            buffer=AmbisonicBuffer(sample_count,channelCount);

            return true;
        }

        void CopyStream(const AudioChannel &src,uint32 channel,uint32 sample_count)
        {
            AudioChannel &dst=buffer[channel];

            std::memcpy(dst.begin(),src.begin(),sample_count*sizeof(float));
        }

        void AddStream(const AudioChannel &src,uint32 channel,uint32 sample_count,uint32 offset=0)
        {
            AudioChannel &dst=buffer[channel];

            for(uint32 i=0;i<sample_count;i++)
                dst[offset+i]+=src[i];
        }

        void GetStream(AudioChannel &dst,uint32 channel,uint32 sample_count)
        {
            const AudioChannel &src=buffer[channel];

            std::memcpy(dst.begin(),src.begin(),sample_count*sizeof(float));
        }

        const AudioChannel &GetBufferChannel(uint32 channel)const{return buffer[channel];}
        AudioChannel &GetBufferChannel(uint32 channel){return buffer[channel];}

        float GetSample(uint32 channel,uint32 index)const{return buffer[channel][index];}
        void SetSample(uint32 channel,uint32 index,float sample){buffer[channel][index]=sample;}

        AmbisonicBuffer *GetBuffer(){return &buffer;}
        const AmbisonicBuffer *GetBuffer()const{return &buffer;}

        BFormat &operator=(const BFormat &other)
        {
            if(this!=&other)
            {
                order=other.order;
                is3D=other.is3D;
                channelCount=other.channelCount;
                buffer=other.buffer;
            }

            return *this;
        }

        bool operator==(const BFormat &other)const
        {
            if(channelCount!=other.channelCount||buffer.GetFrameCount()!=other.buffer.GetFrameCount())
                return false;

            for(uint32 c=0;c<channelCount;c++)
            {
                const AudioChannel &a=buffer[c];
                const AudioChannel &b=other.buffer[c];

                for(uint32 i=0;i<a.size();i++)
                    if(a[i]!=b[i])
                        return false;
            }

            return true;
        }

        bool operator!=(const BFormat &other)const{return !(*this==other);}

        BFormat &operator+=(const BFormat &other)
        {
            for(uint32 c=0;c<channelCount;c++)
                buffer[c]+=other.buffer[c];

            return *this;
        }

        BFormat &operator-=(const BFormat &other)
        {
            for(uint32 c=0;c<channelCount;c++)
                buffer[c]-=other.buffer[c];

            return *this;
        }

        BFormat &operator*=(const BFormat &other)
        {
            for(uint32 c=0;c<channelCount;c++)
                buffer[c]*=other.buffer[c];

            return *this;
        }

        BFormat &operator+=(float value)
        {
            for(uint32 c=0;c<channelCount;c++)
            {
                AudioChannel &ch=buffer[c];

                for(uint32 i=0;i<ch.size();i++)
                    ch[i]+=value;
            }

            return *this;
        }

        BFormat &operator-=(float value)
        {
            for(uint32 c=0;c<channelCount;c++)
            {
                AudioChannel &ch=buffer[c];

                for(uint32 i=0;i<ch.size();i++)
                    ch[i]-=value;
            }

            return *this;
        }

        BFormat &operator*=(float value)
        {
            for(uint32 c=0;c<channelCount;c++)
                buffer[c]*=value;

            return *this;
        }

    private:
        AmbisonicBuffer buffer;
    };
}//namespace hgl::audio
