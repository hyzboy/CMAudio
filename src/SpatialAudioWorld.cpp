#include<hgl/audio/SpatialAudioWorld.h>
#include<hgl/audio/AudioSource.h>
#include<hgl/audio/Listener.h>
#include<hgl/audio/ReverbPreset.h>
#include<hgl/al/efx.h>
#include<hgl/time/Time.h>
#include<hgl/math/Clamp.h>

#include <algorithm>
#include <cmath>

namespace hgl
{
    using namespace openal;

    // 常量定义
    static constexpr float DEFAULT_REF_DISTANCE = 1.0f;
    static constexpr float DEFAULT_MAX_DISTANCE = 10000.0f;
    static constexpr double MIN_TIME_DIFF = 0.0001;  // 避免除以非常小的数导致数值不稳定
    static constexpr double FADE_DURATION = 0.02;    // 淡入淡出持续时间（20毫秒）
    static constexpr double FADE_SILENCE_THRESHOLD = 0.001;  // 判定为静音的增益阈值
    static constexpr double VELOCITY_SMOOTHING_FACTOR = 0.3;  // 速度平滑系数（低通滤波器强度，0-1范围：值越小越平滑但响应慢，值越大越灵敏但平滑效果弱）
    static constexpr double VOICE_STEAL_GAIN_REDUCTION = 0.1;  // 音源抢占时的增益降低系数（降低到原来的10%以避免爆音）

    /**
     * 计算指定音源相对于监听者的音量
     */
    const double GetGain(AudioListener *l,SpatialAudioSource *s)
    {
        if(!l||!s)return(0);

        if(s->gain<=0)return(0);        // 本身音量为0
        
        // 参数验证：无效的距离参数返回0（静音）而非1，避免掩盖配置错误
        if(s->ref_distance<=0.0f)return(0);
        if(s->max_distance<=s->ref_distance)return(0);

        float distance;

        const Vector3f &        lpos=l->GetPosition();

        distance=math::Length(lpos,s->cur_pos);

        if(s->distance_model==AL_INVERSE_DISTANCE_CLAMPED||s->distance_model==AL_INVERSE_DISTANCE)
        {
            if(s->distance_model==AL_INVERSE_DISTANCE_CLAMPED)
                if(distance<=s->ref_distance)
                    return 1;

            // 使用 std::clamp 进行边界限定
            distance = std::clamp(distance, s->ref_distance, s->max_distance);

            return s->ref_distance/(s->ref_distance+s->rolloff_factor*(distance-s->ref_distance));
        }
        else
        if(s->distance_model==AL_LINEAR_DISTANCE_CLAMPED)
        {
            // 使用 std::clamp 进行边界限定
            distance = std::clamp(distance, s->ref_distance, s->max_distance);

            float gain = 1-s->rolloff_factor*(distance-s->ref_distance)/(s->max_distance-s->ref_distance);
            return std::clamp(gain, 0.0f, 1.0f);  // 钳位到 [0, 1]
        }
        else
        if(s->distance_model==AL_LINEAR_DISTANCE)
        {
            distance = std::min(distance, s->max_distance);
            float gain = 1-s->rolloff_factor*(distance-s->ref_distance)/(s->max_distance-s->ref_distance);
            return std::clamp(gain, 0.0f, 1.0f);  // 钳位到 [0, 1]
        }
        else
        if(s->distance_model==AL_EXPONENT_DISTANCE)
        {
            return std::pow(distance/s->ref_distance, -s->rolloff_factor);
        }
        else
        if(s->distance_model==AL_EXPONENT_DISTANCE_CLAMPED)
        {
            // 使用 std::clamp 进行边界限定
            distance = std::clamp(distance, s->ref_distance, s->max_distance);

            return std::pow(distance/s->ref_distance, -s->rolloff_factor);
        }
        else
            return 1;
    }

    /**
     * 音频场景管理类构造函数
     * @param max_source 最大音源数量
     * @param al 监听者
     */
    SpatialAudioWorld::SpatialAudioWorld(int max_source,AudioListener *al)
    {
        source_pool.Reserve(max_source);

        listener=al;

        ref_distance=DEFAULT_REF_DISTANCE;
        max_distance=DEFAULT_MAX_DISTANCE;
        
        // 初始化混响相关变量
        aux_effect_slot=0;
        reverb_effect=0;
        reverb_enabled=false;
    }

    /**
     * 音频场景管理类析构函数
     * 清理所有资源，包括音源和OpenAL混响效果
     */
    SpatialAudioWorld::~SpatialAudioWorld()
    {
        CloseReverb();  // 显式释放OpenAL混响资源
        Clear();        // 释放所有空间音源
    }

    SpatialAudioSource *SpatialAudioWorld::Create(const SpatialAudioSourceConfig &config)
    {
        if(!config.buffer)
            return(nullptr);

        scene_mutex.Lock();

        // 使用场景的默认距离参数更新配置
        SpatialAudioSourceConfig finalConfig = config;
        
        // 如果配置中的距离参数为结构体默认值，则使用场景的默认值
        if(finalConfig.ref_distance == DEFAULT_REF_DISTANCE && ref_distance != DEFAULT_REF_DISTANCE)
            finalConfig.ref_distance = ref_distance;
        if(finalConfig.max_distance == DEFAULT_MAX_DISTANCE && max_distance != DEFAULT_MAX_DISTANCE)
            finalConfig.max_distance = max_distance;
        
        // 如果 distance_model 为 0，使用默认的衰减模型
        if(finalConfig.distance_model == 0)
            finalConfig.distance_model = AL_INVERSE_DISTANCE_CLAMPED;
        
        // 使用最终的配置结构体创建音源
        SpatialAudioSource *asi = new SpatialAudioSource(finalConfig);

        // 在解锁前添加到列表，确保原子性
        source_list.Add(asi);

        scene_mutex.Unlock();

        return asi;
    }

    void SpatialAudioWorld::Delete(SpatialAudioSource *asi)
    {
        if(!asi)return;

        scene_mutex.Lock();
        
        ToMute(asi);

        source_list.Delete(asi);
        
        delete asi;  // 释放音源对象
        
        scene_mutex.Unlock();
    }

    void SpatialAudioWorld::Clear()
    {
        scene_mutex.Lock();
        
        // 先释放所有音源对象
        int count = source_list.GetCount();
        SpatialAudioSource **items = source_list.GetData();
        
        for(int i = 0; i < count; i++)
        {
            if(items[i])
            {
                ToMute(items[i]);
                delete items[i];  // 释放每个音源对象
            }
        }
        
        source_list.Clear();
        source_pool.Clear();
        
        scene_mutex.Unlock();
    }

    bool SpatialAudioWorld::ToMute(SpatialAudioSource *asi)
    {
        if(!asi)return(false);
        if(!asi->source)return(false);

        OnToMute(asi);

        // 启动淡出效果
        asi->is_fading = true;
        asi->fade_start_time = cur_time;
        asi->fade_duration = FADE_DURATION;
        asi->fade_start_gain = asi->source->GetGain();
        asi->fade_target_gain = 0.0;

        return(true);
    }

    bool SpatialAudioWorld::ToHear(SpatialAudioSource *asi)
    {
        if(!asi)return(false);
        if(!asi->buffer)return(false);

        if(asi->start_play_time>cur_time)       // 还没到开始播放时间
            return(false);

        double time_off=0;

        if(asi->start_play_time>0
         &&asi->start_play_time<=cur_time)      // 修复：使用 <= 以处理精确时间匹配
        {
            time_off=cur_time-asi->start_play_time;

            if(time_off>=asi->buffer->GetTime())     // 超过整个音频时长
            {
                if(!asi->loop)                  // 不循环播放
                {
                    asi->is_play=false;         // 不再播放
                    return(false);
                }
                else                            // 循环播放
                {
                    const int count=int(time_off/asi->buffer->GetTime());        // 计算超出的循环次数并取整

                    time_off-=asi->buffer->GetTime()*count;                      // 计算单次的偏移时间
                }
            }
        }

        if(!asi->source)
        {
            if(!source_pool.GetOrCreate(asi->source))
            {
                // 物理音源耗尽，尝试进行音源抢占（voice stealing）
                // 基于 gain * priority 找到当前优先级最低的音源
                SpatialAudioSource *lowest_priority_source = nullptr;
                double lowest_score = asi->gain * asi->priority;  // 当前音源的调度分数
                
                int count = source_list.GetCount();
                SpatialAudioSource **items = source_list.GetData();
                
                for(int i = 0; i < count; i++)
                {
                    SpatialAudioSource *candidate = items[i];
                    
                    // 只考虑已分配物理音源且正在播放的音源
                    if(candidate && candidate->source && candidate != asi)
                    {
                        double candidate_score = candidate->gain * candidate->priority;
                        
                        // 如果找到优先级更低的音源，记录它
                        if(candidate_score < lowest_score)
                        {
                            lowest_score = candidate_score;
                            lowest_priority_source = candidate;
                        }
                    }
                }
                
                // 如果找到了优先级更低的音源，抢占它的物理音源
                if(lowest_priority_source)
                {
                    asi->source = lowest_priority_source->source;
                    lowest_priority_source->source = nullptr;
                    
                    // 立即降低增益并停止，避免爆音（比完整淡出更快但比直接停止更平滑）
                    float current_gain = asi->source->GetGain();
                    if(current_gain > VOICE_STEAL_GAIN_REDUCTION)
                    {
                        asi->source->SetGain(current_gain * VOICE_STEAL_GAIN_REDUCTION);
                    }
                    
                    asi->source->Stop();
                    asi->source->Unlink();
                }
                else
                {
                    // 没有可抢占的音源，无法播放
                    return(false);
                }
            }
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
        
        // 启动淡入效果（在播放开始之前设置）
        asi->is_fading = true;
        asi->fade_start_time = cur_time;
        asi->fade_duration = FADE_DURATION;
        asi->fade_start_gain = 0.0;
        asi->fade_target_gain = asi->gain;
        asi->source->SetGain(0.0);  // 从0开始淡入
        
        asi->source->Play(asi->loop);

        OnToHear(asi);

        return(true);
    }

    bool SpatialAudioWorld::UpdateSource(SpatialAudioSource *asi)
    {
        if(!asi)return(false);
        if(!asi->source)return(false);

        // 处理淡入淡出效果
        if(asi->is_fading)
        {
            double elapsed = cur_time - asi->fade_start_time;
            
            if(elapsed >= asi->fade_duration)
            {
                // 淡入淡出完成
                asi->source->SetGain(asi->fade_target_gain);
                asi->is_fading = false;
                
                // 如果是淡出到静音，现在停止并释放音源
                if(asi->fade_target_gain <= FADE_SILENCE_THRESHOLD)  // 使用命名常量
                {
                    asi->source->Stop();
                    asi->source->Unlink();
                    source_pool.Release(asi->source);
                    asi->source = nullptr;
                    return(true);
                }
            }
            else
            {
                // 计算当前增益（线性插值）
                double t = elapsed / asi->fade_duration;
                double current_gain = asi->fade_start_gain + (asi->fade_target_gain - asi->fade_start_gain) * t;
                asi->source->SetGain(current_gain);
            }
        }

        if(asi->source->GetState()==AL_STOPPED)    // 停止播放状态
        {
            // 如果正在淡出，不要中断，让淡出完成
            if(asi->is_fading && asi->fade_target_gain <= FADE_SILENCE_THRESHOLD)
                return(true);
                
            if(!asi->loop)                  // 不是循环播放
            {
                if(OnStopped(asi))
                    ToMute(asi);

                return(true);
            }
            else
            {
                // 循环播放：先触发事件，允许用户决定是否继续
                bool continue_play = OnStopped(asi);
                if(continue_play)
                {
                    if(!asi->source->Play())  // 尝试继续播放
                    {
                        // 播放失败，释放音源
                        ToMute(asi);
                    }
                }
                else
                {
                    // 用户不希望继续播放，释放音源
                    ToMute(asi);
                }
            }
        }

        if(asi->doppler_factor>0)                   // 需要多普勒效果
        {
            if(asi->last_pos!=asi->cur_pos)         // 位置发生变化
            {
                // 检查时间差，避免除以零或数值不稳定
                double time_diff = asi->cur_time - asi->last_time;
                if(time_diff > MIN_TIME_DIFF)       // 使用最小时间阈值避免数值问题
                {
                    // 计算当前帧的速度矢量
                    Vector3f raw_velocity;
                    raw_velocity.x = (asi->cur_pos.x - asi->last_pos.x) / time_diff;
                    raw_velocity.y = (asi->cur_pos.y - asi->last_pos.y) / time_diff;
                    raw_velocity.z = (asi->cur_pos.z - asi->last_pos.z) / time_diff;
                    
                    // 应用低通滤波平滑速度，防止帧率波动导致的音调抖动
                    // 使用指数移动平均: smoothed = smoothed * (1 - alpha) + raw * alpha
                    const double smooth_factor = VELOCITY_SMOOTHING_FACTOR;
                    const double retain_factor = 1.0 - smooth_factor;
                    asi->smoothed_velocity.x = asi->smoothed_velocity.x * retain_factor + raw_velocity.x * smooth_factor;
                    asi->smoothed_velocity.y = asi->smoothed_velocity.y * retain_factor + raw_velocity.y * smooth_factor;
                    asi->smoothed_velocity.z = asi->smoothed_velocity.z * retain_factor + raw_velocity.z * smooth_factor;
                    
                    // 设置平滑后的矢量速度（OpenAL会自动计算多普勒效果）
                    asi->source->SetVelocity(asi->smoothed_velocity);
                    
                    // 计算标量速度用于记录
                    asi->move_speed = math::Length(asi->last_pos, asi->cur_pos) / time_diff;
                }
            }

            if(cur_time>asi->cur_time)          // 更新时间和位置
            {
                asi->last_pos=asi->cur_pos;
                asi->last_time=asi->cur_time;
            }
        }

        return(true);
    }

    /**
     * 刷新处理
     * @param ct 当前时间
     * @return 监听者仍能听到的音源数量
     * @return -1 出错
     */
    int SpatialAudioWorld::Update(const double &ct)
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
            cur_time=GetTimeSec();

        float new_gain;
        int hear_count=0;

        SpatialAudioSource **ptr=source_list.GetData();

        for(int i=0;i<count;i++)
        {
            if(!(*ptr)->is_play)
            {
                if((*ptr)->source)          // 还有绑定的音源
                    ToMute(*ptr);

                ++ptr;
                continue;   // 不需要播放的音源
            }

            new_gain=OnCheckGain(*ptr);

            if(new_gain<=0)                 // 听不到声音
            {
                if((*ptr)->last_gain>0)     // 之前可以听到
                    ToMute(*ptr);
                else
                    OnContinuedMute(*ptr);  // 之前就听不到
            }
            else
            {
                if((*ptr)->last_gain<=0)
                {
                    if(!ToHear(*ptr))       // 之前没声，尝试转为播放
                        new_gain=0;         // 没有足够可用音源或已播放结束，仍然听不到
                }
                else
                {
                    UpdateSource(*ptr);     // 刷新音源处理
                    OnContinuedHear(*ptr);  // 持续可听
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
    bool SpatialAudioWorld::InitReverb()
    {
        scene_mutex.Lock();

        if(!alGenAuxiliaryEffectSlots || !alGenEffects)
        {
            scene_mutex.Unlock();
            return false;  // EFX 不可用
        }

        // 创建辅助效果槽
        alGenAuxiliaryEffectSlots(1, &aux_effect_slot);
        if(alGetError() != AL_NO_ERROR)
        {
            scene_mutex.Unlock();
            return false;
        }

        // 创建混响效果
        alGenEffects(1, &reverb_effect);
        if(alGetError() != AL_NO_ERROR)
        {
            alDeleteAuxiliaryEffectSlots(1, &aux_effect_slot);
            aux_effect_slot = 0;
            scene_mutex.Unlock();
            return false;
        }

        // 设置为混响类型
        alEffecti(reverb_effect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
        if(alGetError() != AL_NO_ERROR)
        {
            CloseReverb();
            scene_mutex.Unlock();
            return false;
        }

        // 设置默认混响参数（通用预设）
        SetReverbPreset(AudioReverbPreset::Generic);

        reverb_enabled = true;
        
        scene_mutex.Unlock();
        return true;
    }

    /**
     * 关闭混响系统
     */
    void SpatialAudioWorld::CloseReverb()
    {
        scene_mutex.Lock();
        
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
        
        scene_mutex.Unlock();
    }

    /**
     * 将混响预设应用到效果
     * @param preset 混响预设属性结构
     */
    void SpatialAudioWorld::ApplyReverbPreset(const AudioReverbPresetProperties &preset)
    {
        if(!alEffectf || reverb_effect == 0)
            return;

        alEffectf(reverb_effect, AL_REVERB_DENSITY, preset.Density);
        alEffectf(reverb_effect, AL_REVERB_DIFFUSION, preset.Diffusion);
        alEffectf(reverb_effect, AL_REVERB_GAIN, preset.Gain);
        alEffectf(reverb_effect, AL_REVERB_GAINHF, preset.GainHF);
        alEffectf(reverb_effect, AL_REVERB_DECAY_TIME, preset.DecayTime);
        alEffectf(reverb_effect, AL_REVERB_DECAY_HFRATIO, preset.DecayHFRatio);
        alEffectf(reverb_effect, AL_REVERB_REFLECTIONS_GAIN, preset.ReflectionsGain);
        alEffectf(reverb_effect, AL_REVERB_REFLECTIONS_DELAY, preset.ReflectionsDelay);
        alEffectf(reverb_effect, AL_REVERB_LATE_REVERB_GAIN, preset.LateReverbGain);
        alEffectf(reverb_effect, AL_REVERB_LATE_REVERB_DELAY, preset.LateReverbDelay);
        alEffectf(reverb_effect, AL_REVERB_AIR_ABSORPTION_GAINHF, preset.AirAbsorptionGainHF);
        alEffectf(reverb_effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, preset.RoomRolloffFactor);
        alEffecti(reverb_effect, AL_REVERB_DECAY_HFLIMIT, preset.DecayHFLimit);
    }

    /**
     * 设置混响预设（使用 OpenAL Soft 官方预设）
     * @param preset 预设枚举值
     * @return 是否成功设置
     */
    bool SpatialAudioWorld::SetReverbPreset(AudioReverbPreset preset)
    {
        scene_mutex.Lock();
        
        if(!alEffectf || reverb_effect == 0)
        {
            scene_mutex.Unlock();
            return false;
        }

        // 获取预设属性
        const AudioReverbPresetProperties *props = GetAudioReverbPresetProperties(preset);
        if(!props)
        {
            scene_mutex.Unlock();
            return false;
        }

        // 应用预设
        ApplyReverbPreset(*props);

        // 将效果绑定到效果槽
        if(alAuxiliaryEffectSloti)
            alAuxiliaryEffectSloti(aux_effect_slot, AL_EFFECTSLOT_EFFECT, reverb_effect);

        bool result = (alGetError() == AL_NO_ERROR);
        
        scene_mutex.Unlock();
        return result;
    }

    /**
     * 启用/禁用混响
     * @param enable 是否启用
     * @return 是否成功
     */
    bool SpatialAudioWorld::EnableReverb(bool enable)
    {
        scene_mutex.Lock();
        
        if(aux_effect_slot == 0)
        {
            scene_mutex.Unlock();
            return false;
        }

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

        bool result = (alGetError() == AL_NO_ERROR);
        
        scene_mutex.Unlock();
        return result;
    }
}//namespace hgl
