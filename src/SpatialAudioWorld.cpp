#include<hgl/audio/SpatialAudioWorld.h>
#include<hgl/audio/AudioSource.h>
#include<hgl/audio/Listener.h>
#include<hgl/audio/ReverbPreset.h>
#include<hgl/al/efx.h>
#include<hgl/time/Time.h>
#include<hgl/math/Clamp.h>

#include <algorithm>
#include <cmath>
#include <cstdint>  // For UINT64_MAX

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
    static constexpr float VOICE_STEAL_MIN_GAIN_THRESHOLD = 0.01f;  // 音源抢占时需要降低增益的最小阈值
    
    // 分层更新管理常量
    static constexpr double IMPORTANCE_AUDIBLE_GAIN_WEIGHT = 0.4;  // 实际可听增益权重（最重要）
    static constexpr double IMPORTANCE_PRIORITY_WEIGHT = 0.3;      // 优先级权重
    static constexpr double IMPORTANCE_VELOCITY_WEIGHT = 0.2;      // 速度权重（移动物体更重要）
    static constexpr double IMPORTANCE_DISTANCE_WEIGHT = 0.1;      // 距离权重（作为辅助因素）
    static constexpr double MAX_EXPECTED_PRIORITY = 2.0;           // 预期最大优先级（用于归一化）
    static constexpr double HIGH_SPEED_THRESHOLD = 10.0;           // 高速阈值（单位/秒，用于归一化）
    static constexpr uint TIER1_UPDATE_INTERVAL = 1;               // 高重要性：每帧更新
    static constexpr uint TIER2_UPDATE_INTERVAL = 2;               // 中等重要性：每2帧更新
    static constexpr uint TIER3_UPDATE_INTERVAL = 5;               // 低重要性：每5帧更新
    static constexpr double TIER1_THRESHOLD = 0.6;                 // 高重要性阈值
    static constexpr double TIER2_THRESHOLD = 0.3;                 // 中等重要性阈值
    
    // 编译时检查权重总和为1.0（使用精确比较，因为这些是编译时常量）
    constexpr double weight_sum = IMPORTANCE_AUDIBLE_GAIN_WEIGHT + IMPORTANCE_PRIORITY_WEIGHT + 
                                  IMPORTANCE_VELOCITY_WEIGHT + IMPORTANCE_DISTANCE_WEIGHT;
    static_assert(weight_sum == 1.0, "Importance weights must sum to exactly 1.0");

    /**
     * 计算音源的综合重要性分数
     * @param asi 音源
     * @param audible_gain 实际可听增益（经过距离衰减后）
     * @param listener_pos 监听者位置
     * @return 重要性分数(0-1范围，1为最重要)
     */
    static double CalculateImportance(const SpatialAudioSource *asi, double audible_gain, const Vector3f &listener_pos)
    {
        if (!asi) return 0.0;
        
        // 1. 实际可听增益因子（最重要，这是用户真正听到的音量）
        double audible_gain_factor = std::min(audible_gain, 1.0);
        
        // 2. 优先级因子（归一化到0-1范围）
        double priority_factor = std::min(static_cast<double>(asi->priority) / MAX_EXPECTED_PRIORITY, 1.0);
        
        // 3. 速度因子（移动的音源更重要，如接近的敌人、飞过的子弹等）
        // 注意：仅在启用多普勒时考虑速度，因为只有这时才计算和更新 move_speed
        // 对于不使用多普勒的场景，可以通过提高优先级来补偿
        double velocity_factor = 0.0;
        if (asi->doppler_factor > 0)  // 只有启用多普勒效果时才考虑速度
        {
            double speed = asi->move_speed;
            // 将速度归一化
            velocity_factor = std::min(speed / HIGH_SPEED_THRESHOLD, 1.0);
        }
        
        // 4. 距离因子（作为辅助，距离越近可能意味着更需要精确更新）
        float distance = math::Length(listener_pos, asi->cur_pos);
        double distance_factor = 1.0;
        if (asi->max_distance > 0)
        {
            distance_factor = 1.0 - std::min(distance / asi->max_distance, 1.0f);
        }
        
        // 综合重要性 = 各因子加权和
        double importance = audible_gain_factor * IMPORTANCE_AUDIBLE_GAIN_WEIGHT +
                           priority_factor * IMPORTANCE_PRIORITY_WEIGHT +
                           velocity_factor * IMPORTANCE_VELOCITY_WEIGHT +
                           distance_factor * IMPORTANCE_DISTANCE_WEIGHT;
        
        return importance;
    }

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
        
        update_frame_counter=0;

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
                    AudioSource *stolen_source = lowest_priority_source->source;
                    
                    // 立即降低被抢占音源的增益，避免爆音（比完整淡出更快但比直接停止更平滑）
                    float current_gain = stolen_source->GetGain();
                    if(current_gain > VOICE_STEAL_MIN_GAIN_THRESHOLD)  // 只在增益足够大时才需要降低
                    {
                        stolen_source->SetGain(current_gain * VOICE_STEAL_GAIN_REDUCTION);
                    }
                    
                    stolen_source->Stop();
                    stolen_source->Unlink();
                    
                    // 将被抢占的物理音源分配给当前音源
                    asi->source = stolen_source;
                    lowest_priority_source->source = nullptr;
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
        
        // 递增帧计数器（用于分层更新）
        update_frame_counter++;
        
        const Vector3f &listener_pos = listener->GetPosition();

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
                    else
                        (*ptr)->last_update_frame = update_frame_counter;  // 记录更新帧
                }
                else
                {
                    // 分层更新：根据综合重要性决定更新频率
                    // 使用实际可听增益（new_gain）而非原始增益，这更准确反映用户听到的音量
                    double importance = CalculateImportance(*ptr, new_gain, listener_pos);
                    uint update_interval;
                    
                    if(importance >= TIER1_THRESHOLD)
                        update_interval = TIER1_UPDATE_INTERVAL;  // 高重要性：每帧更新
                    else if(importance >= TIER2_THRESHOLD)
                        update_interval = TIER2_UPDATE_INTERVAL;  // 中等重要性：每2帧更新
                    else
                        update_interval = TIER3_UPDATE_INTERVAL;  // 低重要性：每5帧更新
                    
                    // 检查是否需要在当前帧更新此音源（使用模运算避免溢出）
                    uint64 frames_since_update = (update_frame_counter >= (*ptr)->last_update_frame) 
                        ? (update_frame_counter - (*ptr)->last_update_frame)
                        : (UINT64_MAX - (*ptr)->last_update_frame + update_frame_counter + 1);  // 处理溢出
                    
                    if (frames_since_update >= update_interval)
                    {
                        UpdateSource(*ptr);     // 刷新音源处理
                        (*ptr)->last_update_frame = update_frame_counter;
                    }
                    
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
