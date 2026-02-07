#include<hgl/audio/AudioSource.h>
#include<hgl/audio/OpenAL.h>
#include<hgl/log/Log.h>

#include <algorithm>

using namespace openal;
namespace hgl
{
    inline void alSourcefv(openal::ALuint sid, openal::ALenum param, const Vector3f &v3f)
    {
        openal::alSourcefv(sid, param, (const openal::ALfloat *)&v3f);
    }

    inline void alGetSourcefv(openal::ALuint sid, openal::ALenum param, Vector3f &v3f)
    {
        openal::alGetSourcefv(sid, param, (openal::ALfloat *)&v3f);
    }

    const unsigned int InvalidIndex=0xFFFFFFFF;

    void AudioSource::InitPrivate()
    {
        index=InvalidIndex;
        Buffer=nullptr;
        direct_filter=0;
        filter_type=AudioFilterType::None;
        filter_gain=1.0f;
        filter_gain_lf=1.0f;
        filter_gain_hf=1.0f;
    }

    /**
    * 音频发声源基类
    * @param create 是否创建发声源
    */
    AudioSource::AudioSource(bool create)
    {
        InitPrivate();

        if(create)Create();
    }

    /**
    * 音频发声源基类
    * @param ab 音频缓冲区
    */
    AudioSource::AudioSource(AudioBuffer *ab)
    {
        InitPrivate();

        Create();

        Link(ab);
    }

    AudioSource::~AudioSource()
    {
        Close();
    }

    double AudioSource::GetCurTime() const
    {
        if(!alGetSourcei)return(0);
        if(index==InvalidIndex)return(0);

        float offset;

        alGetSourcef(index,AL_SEC_OFFSET,&offset);

        return offset;
    }

    void AudioSource::SetCurTime(const double &ct)
    {
        if(!alGetSourcef)return;
        if(index==InvalidIndex)return;

        alSourcef(index,AL_SEC_OFFSET,ct);
    }

    int AudioSource::GetState() const
    {
        if(!alGetSourcei)return(0);
        if(index==InvalidIndex)return(AL_NONE);

        int state;

        alGetSourcei(index,AL_SOURCE_STATE,&state);

        return(state);
    }

    float AudioSource::GetMinGain() const
    {
        if(!alGetSourcef)return(0);
        if(index==InvalidIndex)return(0);

        float min;

        alGetSourcef(index,AL_MIN_GAIN,&min);

        return(min);
    }

    float AudioSource::GetMaxGain() const
    {
        if(!alGetSourcef)return(0);
        if(index==InvalidIndex)return(0);

        float max;

        alGetSourcef(index,AL_MIN_GAIN,&max);

        return(max);
    }

    void AudioSource::SetLoop(bool _loop)
    {
        if(!alSourcei)return;
        if(index==InvalidIndex)return;

        loop=_loop;
        alSourcei(index,AL_LOOPING,loop);
    }

    void AudioSource::SetPitch(float _pitch)
    {
        if(!alSourcef)return;
        if(index==InvalidIndex)return;

        pitch=_pitch;
        alSourcef(index,AL_PITCH,pitch);
    }

    void AudioSource::SetGain(float _gain)
    {
        if(!alSourcef)return;
        if(index==InvalidIndex)return;

        gain=_gain;
        alSourcef(index,AL_GAIN,gain);
    }

    void AudioSource::SetConeGain(float _gain)
    {
        if(!alSourcef)return;
        if(index==InvalidIndex)return;

        cone_gain=_gain;
        alSourcef(index,AL_CONE_OUTER_GAIN,cone_gain);
    }

    void AudioSource::SetPosition(const Vector3f &pos)
    {
        if(!openal::alSourcefv)return;
        if(index==InvalidIndex)return;

        position=pos;
        alSourcefv(index,AL_POSITION,position);
    }

    void AudioSource::SetVelocity(const Vector3f &vel)
    {
        if(!openal::alSourcefv)return;
        if(index==InvalidIndex)return;

        velocity=vel;
        alSourcefv(index,AL_VELOCITY,velocity);
    }

    void AudioSource::SetDirection(const Vector3f &dir)
    {
        if(!openal::alSourcefv)return;
        if(index==InvalidIndex)return;

        direction=dir;
        alSourcefv(index,AL_DIRECTION,direction);
    }

    void AudioSource::SetDistance(const float &ref_distance,const float &max_distance)
    {
        if(!alSourcef)return;
        if(index==InvalidIndex)return;

        ref_dist=ref_distance;
        max_dist=max_distance;
        alSourcef(index,AL_REFERENCE_DISTANCE,ref_dist);
        alSourcef(index,AL_MAX_DISTANCE,max_dist);
    }

    void AudioSource::SetDistanceModel(uint dm)
    {
        if(!alSourcef)return;
        if(index==InvalidIndex)return;
        if(!alDistanceModel)return;

// #define AL_INVERSE_DISTANCE                      0xD001    //倒数距离
// #define AL_INVERSE_DISTANCE_CLAMPED              0xD002    //钳位倒数距离
// #define AL_LINEAR_DISTANCE                       0xD003    //线性距离
// #define AL_LINEAR_DISTANCE_CLAMPED               0xD004    //钳位线性距离
// #define AL_EXPONENT_DISTANCE                     0xD005    //指数距离
// #define AL_EXPONENT_DISTANCE_CLAMPED             0xD006    //钳位指数距离
        if(dm<AL_INVERSE_DISTANCE
         ||dm>AL_EXPONENT_DISTANCE_CLAMPED)return;

        distance_model=dm;
        alDistanceModel(distance_model);
    }

    void AudioSource::SetRolloffFactor(float rf)
    {
        if(!alSourcef)return;
        if(index==InvalidIndex)return;

        rolloff_factor=rf;
        alSourcef(index,AL_ROLLOFF_FACTOR,rolloff_factor);
    }

    void AudioSource::SetConeAngle(const ConeAngle &ca)
    {
        if(!alSourcef)return;
        if(index==InvalidIndex)return;

        angle=ca;
        alSourcef(index,AL_CONE_INNER_ANGLE,ca.inner);
        alSourcef(index,AL_CONE_OUTER_ANGLE,ca.outer);
    }

    void AudioSource::SetDopplerFactor(const float &factor)
    {
        if(!alSourcef)return;
        if(index==InvalidIndex)return;

        doppler_factor=factor;

        alDopplerFactor(doppler_factor);
    }

    void AudioSource::SetDopplerVelocity(const float &velocity)
    {
        if(!alSourcef)return;
        if(index==InvalidIndex)return;

        doppler_velocity=velocity;

        alDopplerVelocity(doppler_velocity);
    }

    void AudioSource::SetAirAbsorptionFactor(const float &factor)
    {
        if(!alSourcef)return;
        if(index==InvalidIndex)return;

        // 空气吸收因子范围: 0.0 (无吸收) 到 10.0 (最大吸收)
        // 默认值为 0.0，高频在远距离会衰减更快
        air_absorption_factor = factor;

        // AL_AIR_ABSORPTION_FACTOR 是 EFX 扩展的一部分
        alSourcef(index, AL_AIR_ABSORPTION_FACTOR, air_absorption_factor);
    }

    bool AudioSource::SetLowpassFilter(const float gain,const float gain_hf)
    {
        if(!alGenFilters || !alFilteri || !alFilterf || !alSourcei)
            return(false);
        if(index==InvalidIndex)return(false);

        const float clamped_gain = std::clamp(gain, 0.0f, 1.0f);
        const float clamped_gain_hf = std::clamp(gain_hf, 0.0f, 1.0f);

        ALuint filter_id = direct_filter;
        if(filter_id==0)
        {
            alGetError();
            alGenFilters(1,&filter_id);
            if(alGetError()!=AL_NO_ERROR)
                return(false);
        }

        alFilteri(filter_id, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
        if(alGetError()!=AL_NO_ERROR)
        {
            if(direct_filter==0 && alDeleteFilters)
                alDeleteFilters(1,&filter_id);
            return(false);
        }

        alFilterf(filter_id, AL_LOWPASS_GAIN, clamped_gain);
        if(alGetError()!=AL_NO_ERROR)
        {
            if(direct_filter==0 && alDeleteFilters)
                alDeleteFilters(1,&filter_id);
            return(false);
        }

        alFilterf(filter_id, AL_LOWPASS_GAINHF, clamped_gain_hf);
        if(alGetError()!=AL_NO_ERROR)
        {
            if(direct_filter==0 && alDeleteFilters)
                alDeleteFilters(1,&filter_id);
            return(false);
        }

        alSourcei(index, AL_DIRECT_FILTER, filter_id);
        if(alGetError()!=AL_NO_ERROR)
        {
            if(direct_filter==0 && alDeleteFilters)
                alDeleteFilters(1,&filter_id);
            return(false);
        }

        direct_filter = filter_id;
        filter_type = AudioFilterType::Lowpass;
        filter_gain = clamped_gain;
        filter_gain_hf = clamped_gain_hf;
        filter_gain_lf = 1.0f;
        return(true);
    }

    bool AudioSource::SetHighpassFilter(const float gain,const float gain_lf)
    {
        if(!alGenFilters || !alFilteri || !alFilterf || !alSourcei)
            return(false);
        if(index==InvalidIndex)return(false);

        const float clamped_gain = std::clamp(gain, 0.0f, 1.0f);
        const float clamped_gain_lf = std::clamp(gain_lf, 0.0f, 1.0f);

        ALuint filter_id = direct_filter;
        if(filter_id==0)
        {
            alGetError();
            alGenFilters(1,&filter_id);
            if(alGetError()!=AL_NO_ERROR)
                return(false);
        }

        alFilteri(filter_id, AL_FILTER_TYPE, AL_FILTER_HIGHPASS);
        if(alGetError()!=AL_NO_ERROR)
        {
            if(direct_filter==0 && alDeleteFilters)
                alDeleteFilters(1,&filter_id);
            return(false);
        }

        alFilterf(filter_id, AL_HIGHPASS_GAIN, clamped_gain);
        if(alGetError()!=AL_NO_ERROR)
        {
            if(direct_filter==0 && alDeleteFilters)
                alDeleteFilters(1,&filter_id);
            return(false);
        }

        alFilterf(filter_id, AL_HIGHPASS_GAINLF, clamped_gain_lf);
        if(alGetError()!=AL_NO_ERROR)
        {
            if(direct_filter==0 && alDeleteFilters)
                alDeleteFilters(1,&filter_id);
            return(false);
        }

        alSourcei(index, AL_DIRECT_FILTER, filter_id);
        if(alGetError()!=AL_NO_ERROR)
        {
            if(direct_filter==0 && alDeleteFilters)
                alDeleteFilters(1,&filter_id);
            return(false);
        }

        direct_filter = filter_id;
        filter_type = AudioFilterType::Highpass;
        filter_gain = clamped_gain;
        filter_gain_lf = clamped_gain_lf;
        filter_gain_hf = 1.0f;
        return(true);
    }

    bool AudioSource::SetBandpassFilter(const float gain,const float gain_lf,const float gain_hf)
    {
        if(!alGenFilters || !alFilteri || !alFilterf || !alSourcei)
            return(false);
        if(index==InvalidIndex)return(false);

        const float clamped_gain = std::clamp(gain, 0.0f, 1.0f);
        const float clamped_gain_lf = std::clamp(gain_lf, 0.0f, 1.0f);
        const float clamped_gain_hf = std::clamp(gain_hf, 0.0f, 1.0f);

        ALuint filter_id = direct_filter;
        if(filter_id==0)
        {
            alGetError();
            alGenFilters(1,&filter_id);
            if(alGetError()!=AL_NO_ERROR)
                return(false);
        }

        alFilteri(filter_id, AL_FILTER_TYPE, AL_FILTER_BANDPASS);
        if(alGetError()!=AL_NO_ERROR)
        {
            if(direct_filter==0 && alDeleteFilters)
                alDeleteFilters(1,&filter_id);
            return(false);
        }

        alFilterf(filter_id, AL_BANDPASS_GAIN, clamped_gain);
        if(alGetError()!=AL_NO_ERROR)
        {
            if(direct_filter==0 && alDeleteFilters)
                alDeleteFilters(1,&filter_id);
            return(false);
        }

        alFilterf(filter_id, AL_BANDPASS_GAINLF, clamped_gain_lf);
        if(alGetError()!=AL_NO_ERROR)
        {
            if(direct_filter==0 && alDeleteFilters)
                alDeleteFilters(1,&filter_id);
            return(false);
        }

        alFilterf(filter_id, AL_BANDPASS_GAINHF, clamped_gain_hf);
        if(alGetError()!=AL_NO_ERROR)
        {
            if(direct_filter==0 && alDeleteFilters)
                alDeleteFilters(1,&filter_id);
            return(false);
        }

        alSourcei(index, AL_DIRECT_FILTER, filter_id);
        if(alGetError()!=AL_NO_ERROR)
        {
            if(direct_filter==0 && alDeleteFilters)
                alDeleteFilters(1,&filter_id);
            return(false);
        }

        direct_filter = filter_id;
        filter_type = AudioFilterType::Bandpass;
        filter_gain = clamped_gain;
        filter_gain_lf = clamped_gain_lf;
        filter_gain_hf = clamped_gain_hf;
        return(true);
    }

    bool AudioSource::SetFilter(const AudioFilterConfig &config)
    {
        if(!config.enable || config.type==AudioFilterType::None)
        {
            DisableFilter();
            return(true);
        }

        switch(config.type)
        {
            case AudioFilterType::Lowpass:
                return SetLowpassFilter(config.gain, config.gain_hf);
            case AudioFilterType::Highpass:
                return SetHighpassFilter(config.gain, config.gain_lf);
            case AudioFilterType::Bandpass:
                return SetBandpassFilter(config.gain, config.gain_lf, config.gain_hf);
            default:
                break;
        }

        return(false);
    }

    void AudioSource::DisableFilter()
    {
        if(index!=InvalidIndex && alSourcei)
            alSourcei(index, AL_DIRECT_FILTER, AL_FILTER_NULL);

        if(direct_filter!=0 && alDeleteFilters)
            alDeleteFilters(1,&direct_filter);

        direct_filter=0;
        filter_type=AudioFilterType::None;
        filter_gain=1.0f;
        filter_gain_lf=1.0f;
        filter_gain_hf=1.0f;
    }

    /**
     * 播放当前音源
     */
    bool AudioSource::Play()
    {
        if(!alSourcePlay)return(false);
        if(index==InvalidIndex)return(false);
        if(!Buffer
          ||Buffer->GetTime()<=0)return(false);

        if(IsPlaying())
            alSourceStop(index);

        alSourcePlay(index);

        pause=false;

        return(!alLastError());
    }

    /**
     * 播放当前音源，并指定是否循环播放
     * @param _loop 是否循环播放
     */
    bool AudioSource::Play(bool _loop)
    {
        if(!alSourcePlay)return(false);
        if(index==InvalidIndex)return(false);
        if(!Buffer
          ||Buffer->GetTime()<=0)return(false);

        if(IsPlaying())
            alSourceStop(index);

        alSourcePlay(index);
        alSourcei(index,AL_LOOPING,loop=_loop);

        pause=false;

        return(!alLastError());
    }

    void AudioSource::Pause()
    {
        if(!alSourcePlay)return;
        if(!alSourcePause)return;

        if(index==InvalidIndex)return;

        if(!pause)
        {
            alSourcePause(index);
            pause=true;
        }
    }

    void AudioSource::Resume()
    {
        if(!alSourcePlay)return;
        if(!alSourcePause)return;

        if(index==InvalidIndex)return;

        if(pause)
        {
            alSourcePlay(index);
            pause=false;
        }
    }

    void AudioSource::Stop()
    {
        if(!alSourceStop)return;
        if(index==InvalidIndex)return;

        alSourceStop(index);
    }

    void AudioSource::Rewind()
    {
        if(!alSourceRewind)return;
        if(index==InvalidIndex)return;

        alSourceRewind(index);
    }

    /**
    * 创建一个发声源
    * @return 发声源是否创建成功
    */
    bool AudioSource::Create()
    {
        if(!alGenSources)
        {
            LogInfo(OS_TEXT("OpenAL/EE 还未初始化!"));
            return(false);
        }

        if(index!=InvalidIndex)Close();

        alGetError();        //清空错误

        alGenSources(1,&index);

        if(alLastError())
        {
            index=InvalidIndex;

            LogInfo(OS_TEXT("无法再创建音频播放源了！"));
            return(false);
        }

        loop=false;

        alGetSourcef    (index,AL_PITCH,                &pitch);
        alGetSourcef    (index,AL_GAIN,                 &gain);
        alGetSourcef    (index,AL_CONE_OUTER_GAIN,      &cone_gain);
        alGetSourcefv   (index,AL_POSITION,             position);
        alGetSourcefv   (index,AL_VELOCITY,             velocity);
        alGetSourcefv   (index,AL_DIRECTION,            direction);
        alGetSourcef    (index,AL_MAX_DISTANCE,         &max_dist);
        alGetSourcef    (index,AL_REFERENCE_DISTANCE,   &ref_dist);
        alGetSourcef    (index,AL_ROLLOFF_FACTOR,       &rolloff_factor);
        alGetSourcef    (index,AL_CONE_INNER_ANGLE,     &angle.inner);
        alGetSourcef    (index,AL_CONE_OUTER_ANGLE,     &angle.outer);

        // 初始化空气吸收因子为默认值 0.0 (无吸收)
        air_absorption_factor = 0.0f;
        alSourcef       (index,AL_AIR_ABSORPTION_FACTOR, air_absorption_factor);

        return(true);
    }

    /**
    * 关闭发声源
    */
    void AudioSource::Close()
    {
        if(!alDeleteSources)return;
        if(index==InvalidIndex)return;

        DisableFilter();
        alSourceStop(index);
        alSourcei(index,AL_BUFFER,0);
        alDeleteSources(1,&index);

        index=InvalidIndex;
    }

    /**
    * 绑定一个音频数据缓冲区到当前发声源上
    * @param buf 要绑定的音频数据缓冲区
    * @return 是否绑定成功
    */
    bool AudioSource::Link(AudioBuffer *buf)
    {
        if(!buf)return(false);
        if(!buf->GetTime())return(false);
        if(!alSourcei)return(false);
        if(index==InvalidIndex)
        {
            if(!Create())return(false);
        }
        else
            Stop();


        alSourcei(index,AL_BUFFER,buf->GetIndex());

        Buffer=buf;

        return(!alLastError());
    }

    /**
    * 解决绑定在当前音频源上的数据缓冲区
    */
    void AudioSource::Unlink()
    {
        if(!alSourcei)return;
        if(index==InvalidIndex)return;

        Buffer=nullptr;

        alSourcei(index,AL_BUFFER,0);
    }
}//namespace hgl
