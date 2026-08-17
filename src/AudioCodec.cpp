#include<hgl/audio/AudioCodec.h>

#include"AudioDecode.h"

namespace hgl::audio
{
    AudioCodec::AudioCodec()
    {
        iface=nullptr;
        enc=nullptr;
        dec=nullptr;
    }

    AudioCodec::~AudioCodec()
    {
        Close();
    }

    bool AudioCodec::Open(const OSString &plugin_name,uint sample_rate,uint channels,uint bitrate)
    {
        Close();

        AudioCodecPlugInInterface *new_iface=new AudioCodecPlugInInterface;

        if(!GetAudioCodecInterface(plugin_name,new_iface))
        {
            delete new_iface;
            return(false);
        }

        int error=0;

        enc=new_iface->OpenEncoder(sample_rate,channels,bitrate,&error);

        if(!enc)
        {
            delete new_iface;
            return(false);
        }

        dec=new_iface->OpenDecoder(sample_rate,channels,&error);

        if(!dec)
        {
            new_iface->CloseEncoder(enc);
            enc=nullptr;
            delete new_iface;
            return(false);
        }

        iface=new_iface;
        return(true);
    }

    void AudioCodec::Close()
    {
        if(iface)
        {
            if(enc)
            {
                iface->CloseEncoder(enc);
                enc=nullptr;
            }

            if(dec)
            {
                iface->CloseDecoder(dec);
                dec=nullptr;
            }

            delete iface;
            iface=nullptr;
        }
    }

    int AudioCodec::Encode(const float *pcm,uint frame_samples,char *packet,uint packet_cap)
    {
        if(!iface||!enc)return(-1);

        return(iface->Encode(enc,pcm,frame_samples,packet,packet_cap));
    }

    int AudioCodec::Decode(const char *packet,int packet_size,float *pcm,uint pcm_cap)
    {
        if(!iface||!dec)return(-1);

        return(iface->Decode(dec,packet,packet_size,pcm,pcm_cap));
    }
}//namespace hgl::audio
