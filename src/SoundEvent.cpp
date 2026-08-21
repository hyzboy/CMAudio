#include<hgl/audio/SoundEvent.h>

#include<cstring>
#include<random>

namespace hgl::audio
{
    namespace
    {
        std::mt19937 &GetRNG()
        {
            static thread_local std::mt19937 rng(std::random_device{}());
            return rng;
        }
    }

    AudioBusType AudioBusTypeFromString(const char *str)
    {
        if(!str||!(*str))return AudioBusType::SFX;

        if     (strcmp(str,"Master")==0)return AudioBusType::Master;
        else if(strcmp(str,"Music" )==0)return AudioBusType::Music;
        else if(strcmp(str,"SFX"   )==0)return AudioBusType::SFX;
        else if(strcmp(str,"Ambient")==0)return AudioBusType::Ambient;
        else if(strcmp(str,"UI"    )==0)return AudioBusType::UI;

        return AudioBusType::SFX;
    }

    const char *AudioBusTypeToString(AudioBusType type)
    {
        switch(type)
        {
            case AudioBusType::Master:  return "Master";
            case AudioBusType::Music:   return "Music";
            case AudioBusType::Ambient: return "Ambient";
            case AudioBusType::UI:      return "UI";
            case AudioBusType::SFX:
            default:                    return "SFX";
        }
    }

    RTPCTarget RTPCTargetFromString(const char *str)
    {
        if(!str||!(*str))return RTPCTarget::Pitch;

        if     (strcmp(str,"pitch"  )==0)return RTPCTarget::Pitch;
        else if(strcmp(str,"gain"   )==0)return RTPCTarget::Gain;
        else if(strcmp(str,"lowpass")==0)return RTPCTarget::Lowpass;
        else if(strcmp(str,"pan"    )==0)return RTPCTarget::Pan;

        return RTPCTarget::Pitch;
    }

    const char *RTPCTargetToString(RTPCTarget target)
    {
        switch(target)
        {
            case RTPCTarget::Gain:   return "gain";
            case RTPCTarget::Lowpass:return "lowpass";
            case RTPCTarget::Pan:    return "pan";
            case RTPCTarget::Pitch:
            default:                 return "pitch";
        }
    }

    float SoundEventConfig::RandomGain()const
    {
        if(max_gain<=min_gain)return min_gain;

        std::uniform_real_distribution<float> dist(min_gain,max_gain);
        return dist(GetRNG());
    }

    float SoundEventConfig::RandomPitch()const
    {
        if(max_pitch<=min_pitch)return min_pitch;

        std::uniform_real_distribution<float> dist(min_pitch,max_pitch);
        return dist(GetRNG());
    }

    const OSString *SoundEventConfig::RandomFile()const
    {
        const int count=files.GetCount();

        if(count<=0)return nullptr;
        if(count==1)return &files.GetString(0);

        std::uniform_int_distribution<int> dist(0,count-1);
        return &files.GetString(dist(GetRNG()));
    }
}//namespace hgl::audio
