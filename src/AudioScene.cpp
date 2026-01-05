#include<hgl/audio/AudioScene.h>
#include<hgl/audio/AudioSource.h>
#include<hgl/audio/Listener.h>
#include<hgl/audio/ReverbPreset.h>
#include<hgl/al/al.h>
#include<hgl/Time.h>
namespace hgl
{
    /**
     * 計算指定音源相對收聽者的音量
     */
    const double GetGain(AudioListener *l,AudioSourceItem *s)
    {
        if(!l||!s)return(0);

        if(s->gain<=0)return(0);        //本身音量就是0
        
        // 添加参数验证，避免除零
        if(s->ref_distance<=0.0f)return(1);
        if(s->max_distance<=s->ref_distance)return(1);

        float distance;

        const Vector3f &        lpos=l->GetPosition();

        distance=length(lpos,s->cur_pos);

        if(s->distance_model==AL_INVERSE_DISTANCE_CLAMPED||s->distance_model==AL_INVERSE_DISTANCE)
        {
            if(s->distance_model==AL_INVERSE_DISTANCE_CLAMPED)
                if(distance<=s->ref_distance)
                    return 1;

            distance = hgl_max(distance,s->ref_distance);
            distance = hgl_min(distance,s->max_distance);

            return s->ref_distance/(s->ref_distance+s->rolloff_factor*(distance-s->ref_distance));
        }
        else
        if(s->distance_model==AL_LINEAR_DISTANCE_CLAMPED)
        {
            distance = hgl_max(distance,s->ref_distance);
            distance = hgl_min(distance,s->max_distance);

            float gain = 1-s->rolloff_factor*(distance-s->ref_distance)/(s->max_distance-s->ref_distance);
            return hgl_max(0.0f, hgl_min(1.0f, gain));  // 钳位到 [0, 1]
        }
        else
        if(s->distance_model==AL_LINEAR_DISTANCE)
        {
            distance = hgl_min(distance,s->max_distance);
            float gain = 1-s->rolloff_factor*(distance-s->ref_distance)/(s->max_distance-s->ref_distance);
            return hgl_max(0.0f, hgl_min(1.0f, gain));  // 钳位到 [0, 1]
        }
        else
        if(s->distance_model==AL_EXPONENT_DISTANCE)
        {
            return pow(distance/s->ref_distance,-s->rolloff_factor);
        }
        else
        if(s->distance_model==AL_EXPONENT_DISTANCE_CLAMPED)
        {
            distance = hgl_max(distance,s->ref_distance);
            distance = hgl_min(distance,s->max_distance);

            return pow(distance/s->ref_distance,-s->rolloff_factor);
        }
        else
            return 1;
    }

    /**
     * 音频场景管理类构造函数
     * @param max_source 最大音源数量
     * @param al 收听者
     */
    AudioScene::AudioScene(int max_source,AudioListener *al)
    {
        source_pool.PreMalloc(max_source);

        listener=al;

        ref_distance=1;
        max_distance=10000;
        
        // 初始化混响相关变量
        aux_effect_slot=0;
        reverb_effect=0;
        reverb_enabled=false;
    }

    AudioSourceItem *AudioScene::Create(AudioBuffer *buf,const Vector3f &pos,const float &gain)
    {
        if(!buf)
            return(nullptr);

        AudioSourceItem *asi=new AudioSourceItem;  // 构造函数已初始化所有字段

        // 设置必要的参数
        asi->buffer=buf;
        asi->gain=gain;
        asi->distance_model=AL_INVERSE_DISTANCE_CLAMPED;
        asi->rolloff_factor=1;
        asi->ref_distance=ref_distance;
        asi->max_distance=max_distance;
        asi->last_pos=pos;
        asi->cur_pos=pos;

        return asi;
    }

    void AudioScene::Delete(AudioSourceItem *asi)
    {
        if(!asi)return;

        scene_mutex.Lock();
        
        ToMute(asi);

        source_list.Delete(asi);
        
        delete asi;  // 修复内存泄漏：释放 AudioSourceItem 对象
        
        scene_mutex.Unlock();
    }

    void AudioScene::Clear()
    {
        scene_mutex.Lock();
        
        // 先释放所有 AudioSourceItem 对象
        int count = source_list.GetCount();
        AudioSourceItem **items = source_list.GetData();
        
        for(int i = 0; i < count; i++)
        {
            if(items[i])
            {
                ToMute(items[i]);
                delete items[i];  // 修复内存泄漏：释放每个 AudioSourceItem
            }
        }
        
        source_list.Clear();
        source_pool.ReleaseAll();
        
        scene_mutex.Unlock();
    }

    bool AudioScene::ToMute(AudioSourceItem *asi)
    {
        if(!asi)return(false);
        if(!asi->source)return(false);

        OnToMute(asi);

        asi->source->Stop();
        asi->source->Unlink();
        source_pool.Release(asi->source);

        asi->source=nullptr;

        return(true);
    }

    bool AudioScene::ToHear(AudioSourceItem *asi)
    {
        if(!asi)return(false);
        if(!asi->buffer)return(false);

        if(asi->start_play_time>cur_time)       //还没到起播时间
            return(false);

        double time_off=0;

        if(asi->start_play_time>0
         &&asi->start_play_time<cur_time)
        {
            time_off=cur_time-asi->start_play_time;

            if(time_off>=asi->buffer->GetTime())     //大于整个音频的时间
            {
                if(!asi->loop)                  //无需循环
                {
                    asi->is_play=false;         //不用放了
                    return(false);
                }
                else                            //循环播放的
                {
                    const int count=int(time_off/asi->buffer->GetTime());        //计算超了几次并取整

                    time_off-=asi->buffer->GetTime()*count;                      //计算单次的偏移时间
                }
            }
        }

        if(!asi->source)
        {
            if(!source_pool.Acquire(asi->source))
                return(false);
        }

        asi->source->Link(asi->buffer);

        asi->source->SetGain(asi->gain);
        asi->source->SetDistanceModel(asi->distance_model);
        asi->source->SetRolloffFactor(asi->rolloff_factor);
        asi->source->SetDistance(asi->ref_distance,asi->max_distance);
        asi->source->SetPosition(asi->cur_pos);
        asi->source->SetConeAngle(asi->cone_angle);
        asi->source->SetVelocity(asi->velocity);
        asi->source->SetDirection(asi->direction);
        asi->source->SetDopplerFactor(asi->doppler_factor);
        asi->source->SetDopplerVelocity(0);
        asi->source->SetAirAbsorptionFactor(asi->air_absorption_factor);

        // 应用混响效果
        if(reverb_enabled && aux_effect_slot != 0 && alSource3i)
        {
            alSource3i(asi->source->GetIndex(), AL_AUXILIARY_SEND_FILTER, aux_effect_slot, 0, AL_FILTER_NULL);
        }

        asi->source->SetCurTime(time_off);
        asi->source->Play(asi->loop);

        OnToHear(asi);

        return(true);
    }

    bool AudioScene::UpdateSource(AudioSourceItem *asi)
    {
        if(!asi)return(false);
        if(!asi->source)return(false);

        if(asi->source->GetState()==AL_STOPPED)    //停播状态
        {
            if(!asi->loop)                  //不是循环播放
            {
                if(OnStopped(asi))
                    ToMute(asi);

                return(true);
            }
            else
            {
                // 循环播放：先触发事件，允许用户控制是否继续
                bool continue_play = OnStopped(asi);
                if(continue_play)
                {
                    if(!asi->source->Play())  // 检查返回值
                    {
                        // 播放失败，释放音源
                        ToMute(asi);
                    }
                }
                else
                {
                    // 用户不希望继续播放
                    ToMute(asi);
                }
            }
        }

        if(asi->doppler_factor>0)                   //需要多普勒效果
        {
            if(asi->last_pos!=asi->cur_pos)         //坐标不一样了
            {
                // 检查时间差，避免除零
                double time_diff = asi->cur_time - asi->last_time;
                if(time_diff > 0)
                {
                    // 计算音源的速度矢量
                    Vector3f velocity;
                    velocity.x = (asi->cur_pos.x - asi->last_pos.x) / time_diff;
                    velocity.y = (asi->cur_pos.y - asi->last_pos.y) / time_diff;
                    velocity.z = (asi->cur_pos.z - asi->last_pos.z) / time_diff;
                    
                    // 设置矢量速度（OpenAL会自动计算多普勒效果）
                    asi->source->SetVelocity(velocity);
                    
                    // 计算标量速度用于记录
                    asi->move_speed = length(asi->last_pos, asi->cur_pos) / time_diff;
                }
            }

            if(cur_time>asi->cur_time)          //更新时间和坐标
            {
                asi->last_pos=asi->cur_pos;
                asi->last_time=asi->cur_time;
            }
        }

        return(true);
    }

    /**
     * 刷新處理
     * @param ct 当前时间
     * @return 收聽者仍可聽到的音源數量
     * @return -1 出錯
     */
    int AudioScene::Update(const double &ct)
    {
        scene_mutex.Lock();
        
        if(!listener)
        {
            scene_mutex.Unlock();
            return(-1);
        }

        const int count=source_list.GetCount();

        if(count<=0)
        {
            scene_mutex.Unlock();
            return 0;
        }

        if(ct!=0)
            cur_time=ct;
        else
            cur_time=GetDoubleTime();

        float new_gain;
        int hear_count=0;

        AudioSourceItem **ptr=source_list.GetData();

        for(int i=0;i<count;i++)
        {
            if(!(*ptr)->is_play)
            {
                if((*ptr)->source)          //还有音源
                    ToMute(*ptr);

                ++ptr;
                continue;   //不需要放的音源
            }

            new_gain=OnCheckGain(*ptr);

            if(new_gain<=0)                 //听不到声音了
            {
                if((*ptr)->last_gain>0)     //之前可以听到
                    ToMute(*ptr);
                else
                    OnContinuedMute(*ptr);  //之前就听不到
            }
            else
            {
                if((*ptr)->last_gain<=0)
                {
                    if(!ToHear(*ptr))       //之前没声
                        new_gain=0;         //没有足够可用音源、或是已经放完了，所以还是听不到
                }
                else
                {
                    UpdateSource(*ptr);     //刷新音源处理
                    OnContinuedHear(*ptr);  //之前就有声音
                }
            }

            (*ptr)->last_gain=new_gain;

            if(new_gain>0)
                ++hear_count;

            ++ptr;
        }

        scene_mutex.Unlock();
        return hear_count;
    }

    /**
     * 初始化混响系统
     * @return 是否成功初始化
     */
    bool AudioScene::InitReverb()
    {
        if(!alGenAuxiliaryEffectSlots || !alGenEffects)
            return false;  // EFX 不可用

        // 创建辅助效果槽
        alGenAuxiliaryEffectSlots(1, &aux_effect_slot);
        if(alGetError() != AL_NO_ERROR)
            return false;

        // 创建混响效果
        alGenEffects(1, &reverb_effect);
        if(alGetError() != AL_NO_ERROR)
        {
            alDeleteAuxiliaryEffectSlots(1, &aux_effect_slot);
            return false;
        }

        // 设置为混响类型
        alEffecti(reverb_effect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
        if(alGetError() != AL_NO_ERROR)
        {
            CloseReverb();
            return false;
        }

        // 设置默认混响参数（通用预设）
        SetReverbPreset(ReverbPreset::Generic);

        reverb_enabled = true;
        return true;
    }

    /**
     * 关闭混响系统
     */
    void AudioScene::CloseReverb()
    {
        reverb_enabled = false;

        if(reverb_effect != 0)
        {
            if(alDeleteEffects)
                alDeleteEffects(1, &reverb_effect);
            reverb_effect = 0;
        }

        if(aux_effect_slot != 0)
        {
            if(alDeleteAuxiliaryEffectSlots)
                alDeleteAuxiliaryEffectSlots(1, &aux_effect_slot);
            aux_effect_slot = 0;
        }
    }

    /**
     * 应用混响预设结构到效果
     * @param preset 混响预设属性结构
     */
    void AudioScene::ApplyReverbPreset(const ReverbPresetProperties &preset)
    {
        if(!alEffectf || reverb_effect == 0)
            return;

        alEffectf(reverb_effect, AL_REVERB_DENSITY, preset.flDensity);
        alEffectf(reverb_effect, AL_REVERB_DIFFUSION, preset.flDiffusion);
        alEffectf(reverb_effect, AL_REVERB_GAIN, preset.flGain);
        alEffectf(reverb_effect, AL_REVERB_GAINHF, preset.flGainHF);
        alEffectf(reverb_effect, AL_REVERB_DECAY_TIME, preset.flDecayTime);
        alEffectf(reverb_effect, AL_REVERB_DECAY_HFRATIO, preset.flDecayHFRatio);
        alEffectf(reverb_effect, AL_REVERB_REFLECTIONS_GAIN, preset.flReflectionsGain);
        alEffectf(reverb_effect, AL_REVERB_REFLECTIONS_DELAY, preset.flReflectionsDelay);
        alEffectf(reverb_effect, AL_REVERB_LATE_REVERB_GAIN, preset.flLateReverbGain);
        alEffectf(reverb_effect, AL_REVERB_LATE_REVERB_DELAY, preset.flLateReverbDelay);
        alEffectf(reverb_effect, AL_REVERB_AIR_ABSORPTION_GAINHF, preset.flAirAbsorptionGainHF);
        alEffectf(reverb_effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, preset.flRoomRolloffFactor);
        alEffecti(reverb_effect, AL_REVERB_DECAY_HFLIMIT, preset.iDecayHFLimit);
    }

    /**
     * 设置混响预设（使用 OpenAL Soft 官方预设）
     * @param preset 预设枚举值
     * @return 是否成功设置
     */
    bool AudioScene::SetReverbPreset(ReverbPreset preset)
    {
        if(!alEffectf || reverb_effect == 0)
            return false;

        // 获取预设属性
        const ReverbPresetProperties *props = GetReverbPresetProperties(preset);
        if(!props)
            return false;

        // 应用预设
        ApplyReverbPreset(*props);

        // 将效果绑定到效果槽
        if(alAuxiliaryEffectSloti)
            alAuxiliaryEffectSloti(aux_effect_slot, AL_EFFECTSLOT_EFFECT, reverb_effect);

        return (alGetError() == AL_NO_ERROR);
    }

    /**
     * 启用/禁用混响
     * @param enable 是否启用
     * @return 是否成功
     */
    bool AudioScene::EnableReverb(bool enable)
    {
        if(aux_effect_slot == 0)
            return false;

        reverb_enabled = enable;

        // 如果禁用，将效果槽设置为 NULL 效果
        if(!enable && alAuxiliaryEffectSloti)
        {
            alAuxiliaryEffectSloti(aux_effect_slot, AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);
        }
        else if(enable && reverb_effect != 0 && alAuxiliaryEffectSloti)
        {
            alAuxiliaryEffectSloti(aux_effect_slot, AL_EFFECTSLOT_EFFECT, reverb_effect);
        }

        return (alGetError() == AL_NO_ERROR);
    }
}//namespace hgl
