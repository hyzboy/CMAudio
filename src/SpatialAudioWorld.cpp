#include<hgl/audio/SpatialAudioWorld.h>
#include<hgl/audio/AudioSource.h>
#include<hgl/audio/AudioListener.h>
#include<hgl/audio/ReverbPreset.h>
#include<hgl/al/efx.h>
#include<hgl/time/Time.h>
#include<hgl/math/Clamp.h>

#include <algorithm>
#include <cmath>
#include <cstdint>  // For UINT64_MAX
#include <limits>

namespace hgl::audio
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

    // 频率相关衰减常量
    static constexpr float FREQ_ATTEN_MIN_GAIN = 0.3f;             // 高频增益最小值（远距离时保留30%高频）
    static constexpr float FREQ_ATTEN_CHANGE_THRESHOLD = 0.05f;    // 滤波器更新阈值（5%变化才更新）

    // 编译时检查权重总和约等于1.0（允许极小的浮点误差）
    constexpr double weight_sum = IMPORTANCE_AUDIBLE_GAIN_WEIGHT + IMPORTANCE_PRIORITY_WEIGHT +
                                  IMPORTANCE_VELOCITY_WEIGHT + IMPORTANCE_DISTANCE_WEIGHT;
    constexpr double weight_eps = std::numeric_limits<double>::epsilon() * 8; // 放宽容差
    static_assert(weight_sum > 1.0 - weight_eps && weight_sum < 1.0 + weight_eps,
                  "Importance weights must sum to ~1.0 (within epsilon)");

    /**
     * 计算音源的综合重要性分数（作为类静态成员定义，访问私有字段）
     * @param spatial_source 音源
     * @param audible_gain 实际可听增益（经过距离衰减后）
     * @param listener_pos 监听者位置
     * @return 重要性分数(0-1范围，1为最重要)
     */
    double SpatialAudioWorld::CalculateImportance(const SpatialAudioSource *spatial_source, double audible_gain, const Vector3f &listener_pos)
    {
        if (!spatial_source) return 0.0;

        // 1. 实际可听增益因子（最重要，这是用户真正听到的音量）
        double audible_gain_factor = std::min(audible_gain, 1.0);

        // 2. 优先级因子（归一化到0-1范围）
        double priority_factor = std::min(static_cast<double>(spatial_source->priority) / MAX_EXPECTED_PRIORITY, 1.0);

        // 3. 速度因子（移动的音源更重要，如接近的敌人、飞过的子弹等）
        // 注意：仅在启用多普勒时考虑速度，因为只有这时才计算和更新 movement_speed
        // 对于不使用多普勒的场景，可以通过提高优先级来补偿
        double velocity_factor = 0.0;
        if (spatial_source->doppler_factor > 0)  // 只有启用多普勒效果时才考虑速度
        {
            double speed = spatial_source->movement_speed;
            // 将速度归一化
            velocity_factor = std::min(speed / HIGH_SPEED_THRESHOLD, 1.0);
        }

        // 4. 距离因子（作为辅助，距离越近可能意味着更需要精确更新）
        float distance = math::Length(listener_pos, spatial_source->current_position);
        double distance_factor = 1.0;
        if (spatial_source->max_distance > 0)
        {
            distance_factor = 1.0 - std::min(distance / spatial_source->max_distance, 1.0f);
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
    double SpatialAudioSource::GetGain(const AudioListener *l)const
    {
        if(!l)return(0);

        if(gain<=0)return(0);        // 本身音量为0

        // 参数验证：无效的距离参数返回0（静音）而非1，避免掩盖配置错误
        if(ref_distance<=0.0f)return(0);
        if(max_distance<=ref_distance)return(0);

        float distance;

        const Vector3f &        lpos=l->GetPosition();

        distance=math::Length(lpos,current_position);

        if(distance_model==AL_INVERSE_DISTANCE_CLAMPED||distance_model==AL_INVERSE_DISTANCE)
        {
            if(distance_model==AL_INVERSE_DISTANCE_CLAMPED)
                if(distance<=ref_distance)
                    return 1;

            // 使用 std::clamp 进行边界限定
            distance = std::clamp(distance, ref_distance, max_distance);

            return ref_distance/(ref_distance+rolloff_factor*(distance-ref_distance));
        }
        else
        if(distance_model==AL_LINEAR_DISTANCE_CLAMPED)
        {
            // 使用 std::clamp 进行边界限定
            distance = std::clamp(distance, ref_distance, max_distance);

            float gain = 1-rolloff_factor*(distance-ref_distance)/(max_distance-ref_distance);
            return std::clamp(gain, 0.0f, 1.0f);  // 钳位到 [0, 1]
        }
        else
        if(distance_model==AL_LINEAR_DISTANCE)
        {
            distance = std::min(distance, max_distance);
            float gain = 1-rolloff_factor*(distance-ref_distance)/(max_distance-ref_distance);
            return std::clamp(gain, 0.0f, 1.0f);  // 钳位到 [0, 1]
        }
        else
        if(distance_model==AL_EXPONENT_DISTANCE)
        {
            return std::pow(distance/ref_distance, -rolloff_factor);
        }
        else
        if(distance_model==AL_EXPONENT_DISTANCE_CLAMPED)
        {
            // 使用 std::clamp 进行边界限定
            distance = std::clamp(distance, ref_distance, max_distance);

            return std::pow(distance/ref_distance, -rolloff_factor);
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
        source_pool.Init(max_source);      // 预分配音源对象池（固定大小）
        spatial_source_pool.Init();         // 初始化空间音源对象池
        
        // 预创建空间音源对象并加入池
        for(int i = 0; i < max_source; i++)
        {
            SpatialAudioSource *spatial_source = new SpatialAudioSource(SpatialAudioSourceConfig());
            spatial_source_pool.AddObject(spatial_source);
        }

        listener=al;

        world_bus=nullptr;

        update_frame_counter=0;

        ref_distance=DEFAULT_REF_DISTANCE;
        max_distance=DEFAULT_MAX_DISTANCE;

        // 初始化混响相关变量
        aux_effect_slot=0;
        reverb_effect=0;
        reverb_enabled=false;

        // 初始化频率相关衰减变量
        frequency_dependent_attenuation=false;
        freq_atten_min_gain_hf=FREQ_ATTEN_MIN_GAIN;

        // 初始化场景级低通参数
        scene_lowpass_enabled=false;
        scene_lowpass_gain=1.0f;
        scene_lowpass_gain_hf=1.0f;

        // 初始化淡入淡出插值类型（默认使用余弦插值，更适合音频）
        fade_interpolation_type=InterpolationType::Cosine;
    }

    void SpatialAudioWorld::SetBus(AudioBus *b)
    {
        scene_mutex.Lock();

        world_bus=b;

        for(SpatialAudioSource *spatial_source : source_list)
            if(spatial_source && spatial_source->source)
                spatial_source->source->SetBus(b);

        scene_mutex.Unlock();
    }

    /**
     * 音频场景管理类析构函数
     * 清理所有资源，包括音源和OpenAL混响效果
     */
    SpatialAudioWorld::~SpatialAudioWorld()
    {
        CloseFrequencyAttenuation();  // 释放频率相关衰减资源
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

        // 从对象池获取对象，或创建新对象
        SpatialAudioSource *spatial_source = spatial_source_pool.Acquire();
        if(!spatial_source)
        {
            // 池空则创建新对象
            spatial_source = new SpatialAudioSource(finalConfig);
            if(!spatial_source)
            {
                scene_mutex.Unlock();
                return nullptr;
            }
        }
        else
        {
            // 重新初始化对象的音频配置（因为对象来自池，可能被前面的用户修改过）
            spatial_source->buffer = finalConfig.buffer;
            spatial_source->loop = finalConfig.loop;
            spatial_source->gain = finalConfig.gain;
            spatial_source->priority = finalConfig.priority;
            spatial_source->distance_model = finalConfig.distance_model;
            spatial_source->rolloff_factor = finalConfig.rolloff_factor;
            spatial_source->doppler_factor = finalConfig.doppler_factor;
            spatial_source->air_absorption_factor = finalConfig.air_absorption_factor;
            spatial_source->ref_distance = finalConfig.ref_distance;
            spatial_source->max_distance = finalConfig.max_distance;
            spatial_source->source = nullptr;  // 初始化物理音源为空
            spatial_source->lowpass_filter = 0;
            spatial_source->last_filter_gain = -1.0f;
            spatial_source->last_filter_gain_hf = -1.0f;
        }

        // 在解锁前添加到列表，确保原子性
        source_list.Add(spatial_source);

        scene_mutex.Unlock();

        return spatial_source;
    }

    void SpatialAudioWorld::Delete(SpatialAudioSource *spatial_source)
    {
        if(!spatial_source)return;

        scene_mutex.Lock();

        ToMute(spatial_source);

        source_list.Delete(spatial_source);

        // 归还到对象池（PointerObjectPool 会保留对象以供重用）
        spatial_source_pool.Release(spatial_source);

        scene_mutex.Unlock();
    }

    void SpatialAudioWorld::Clear()
    {
        scene_mutex.Lock();

        // 先牙渺所有音源对象并归还到池
        for(auto source : source_list)
        {
            if(source)
            {
                ToMute(source);
                // 归还到对象池（PointerObjectPool 会保留对象下次重用）
                spatial_source_pool.Release(source);
            }
        }

        source_list.Clear();

        scene_mutex.Unlock();
    }

    bool SpatialAudioWorld::ToMute(SpatialAudioSource *spatial_source)
    {
        if(!spatial_source)return(false);
        if(!spatial_source->source)return(false);

        OnToMute(spatial_source);

        // 启动淡出效果
        spatial_source->is_fading = true;
        spatial_source->fade_start_time = current_time;
        spatial_source->fade_duration = FADE_DURATION;
        spatial_source->fade_start_gain = spatial_source->source->GetGain();
        spatial_source->fade_target_gain = 0.0;

        // 释放per-source的低通滤波器
        if(spatial_source->lowpass_filter != 0)
        {
            if(alDeleteFilters)
                alDeleteFilters(1, &spatial_source->lowpass_filter);
            spatial_source->lowpass_filter = 0;
        }

        return(true);
    }

    bool SpatialAudioWorld::ToHear(SpatialAudioSource *spatial_source)
    {
        if(!spatial_source)return(false);
        if(!spatial_source->buffer)return(false);

        if(spatial_source->start_play_time>current_time)       // 还没到开始播放时间
            return(false);

        double time_off=0;

        if(spatial_source->start_play_time>0
         &&spatial_source->start_play_time<=current_time)      // 修复：使用 <= 以处理精确时间匹配
        {
            time_off=current_time-spatial_source->start_play_time;

            if(time_off>=spatial_source->buffer->GetTime())     // 超过整个音频时长
            {
                if(!spatial_source->loop)                  // 不循环播放
                {
                    spatial_source->should_play=false;         // 不再播放
                    return(false);
                }
                else                            // 循环播放
                {
                    const int count=int(time_off/spatial_source->buffer->GetTime());        // 计算超出的循环次数并取整

                    time_off-=spatial_source->buffer->GetTime()*count;                      // 计算单次的偏移时间
                }
            }
        }

        if(!spatial_source->source)
        {
            // 从对象池获取音源
            spatial_source->source = source_pool.Acquire();
            if(!spatial_source->source)
            {
                // 物理音源耗尽，尝试进行音源抢占（voice stealing）
                    // 基于 gain * priority 找到当前优先级最低的音源
                    SpatialAudioSource *lowest_priority_source = nullptr;
                    double lowest_score = spatial_source->gain * spatial_source->priority;  // 当前音源的调度分数

                    for(auto candidate : source_list)
                {
                    // 只考虑已分配物理音源且正在播放的音源
                    if(candidate && candidate->source && candidate != spatial_source)
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
                    spatial_source->source = stolen_source;
                    lowest_priority_source->source = nullptr;
                }
                else
                {
                    // 没有可抢占的音源，无法播放
                    return(false);
                }
            }
        }

        if(world_bus)spatial_source->source->SetBus(world_bus);   // 补挂总线（含对象池复用/偷取转移的源）

        spatial_source->source->Link(spatial_source->buffer);

        spatial_source->source->SetGain(spatial_source->gain);
        spatial_source->source->SetDistanceModel(spatial_source->distance_model);
        spatial_source->source->SetRolloffFactor(spatial_source->rolloff_factor);
        spatial_source->source->SetDistance(spatial_source->ref_distance,spatial_source->max_distance);
        spatial_source->source->SetPosition(spatial_source->current_position);
        spatial_source->source->SetConeAngle(spatial_source->cone_angle);
        spatial_source->source->SetVelocity(spatial_source->velocity);
        spatial_source->source->SetDirection(spatial_source->direction);
        spatial_source->source->SetDopplerFactor(spatial_source->doppler_factor);
        spatial_source->source->SetDopplerVelocity(0);
        spatial_source->source->SetAirAbsorptionFactor(spatial_source->air_absorption_factor);

        // 应用混响效果
        if(reverb_enabled && aux_effect_slot != 0 && alSource3i)
        {
            alSource3i(spatial_source->source->GetIndex(), AL_AUXILIARY_SEND_FILTER, aux_effect_slot, 0, AL_FILTER_NULL);
        }

        spatial_source->source->SetPlaybackTime(time_off);

        // 启动淡入效果（在播放开始之前设置）
        spatial_source->is_fading = true;
        spatial_source->fade_start_time = current_time;
        spatial_source->fade_duration = FADE_DURATION;
        spatial_source->fade_start_gain = 0.0;
        spatial_source->fade_target_gain = spatial_source->gain;
        spatial_source->source->SetGain(0.0);  // 从0开始淡入

        spatial_source->source->Play(spatial_source->loop);

        OnToHear(spatial_source);

        return(true);
    }

    bool SpatialAudioWorld::UpdateSource(SpatialAudioSource *spatial_source)
    {
        if(!spatial_source)return(false);
        if(!spatial_source->source)return(false);

        // 处理淡入淡出效果
        if(spatial_source->is_fading)
        {
            double elapsed = current_time - spatial_source->fade_start_time;

            if(elapsed >= spatial_source->fade_duration)
            {
                // 淡入淡出完成
                spatial_source->source->SetGain(spatial_source->fade_target_gain);
                spatial_source->is_fading = false;

                // 如果是淡出到静音，现在停止并释放音源
                if(spatial_source->fade_target_gain <= FADE_SILENCE_THRESHOLD)  // 使用命名常量
                {
                    spatial_source->source->Stop();
                    spatial_source->source->Unlink();
                    source_pool.Release(spatial_source->source);  // 将音源归还到对象池
                    spatial_source->source = nullptr;
                    return(true);
                }
            }
            else
            {
                // 计算当前增益（使用配置的插值算法）
                double t = elapsed / spatial_source->fade_duration;
                double current_gain = Interpolation::Interpolate(
                    fade_interpolation_type,
                    (float)spatial_source->fade_start_gain,
                    (float)spatial_source->fade_target_gain,
                    (float)t
                );
                spatial_source->source->SetGain(current_gain);
            }
        }

        if(spatial_source->source->GetState()==AL_STOPPED)    // 停止播放状态
        {
            // 如果正在淡出，不要中断，让淡出完成
            if(spatial_source->is_fading && spatial_source->fade_target_gain <= FADE_SILENCE_THRESHOLD)
                return(true);

            if(!spatial_source->loop)                  // 不是循环播放
            {
                if(OnStopped(spatial_source))
                    ToMute(spatial_source);

                return(true);
            }
            else
            {
                // 循环播放：先触发事件，允许用户决定是否继续
                bool continue_play = OnStopped(spatial_source);
                if(continue_play)
                {
                    if(!spatial_source->source->Play())  // 尝试继续播放
                    {
                        // 播放失败，释放音源
                        ToMute(spatial_source);
                    }
                }
                else
                {
                    // 用户不希望继续播放，释放音源
                    ToMute(spatial_source);
                }
            }
        }

        if(spatial_source->doppler_factor>0)                   // 需要多普勒效果
        {
            if(spatial_source->last_position!=spatial_source->current_position)         // 位置发生变化
            {
                // 检查时间差，避免除以零或数值不稳定
                double time_diff = spatial_source->current_position_time - spatial_source->last_position_time;
                if(time_diff > MIN_TIME_DIFF)       // 使用最小时间阈值避免数值问题
                {
                    // 计算当前帧的速度矢量
                    Vector3f raw_velocity;
                    raw_velocity.x = (spatial_source->current_position.x - spatial_source->last_position.x) / time_diff;
                    raw_velocity.y = (spatial_source->current_position.y - spatial_source->last_position.y) / time_diff;
                    raw_velocity.z = (spatial_source->current_position.z - spatial_source->last_position.z) / time_diff;

                    // 应用低通滤波平滑速度，防止帧率波动导致的音调抖动
                    // 使用指数移动平均: smoothed = smoothed * (1 - alpha) + raw * alpha
                    const double smooth_factor = VELOCITY_SMOOTHING_FACTOR;
                    const double retain_factor = 1.0 - smooth_factor;
                    spatial_source->smoothed_velocity.x = spatial_source->smoothed_velocity.x * retain_factor + raw_velocity.x * smooth_factor;
                    spatial_source->smoothed_velocity.y = spatial_source->smoothed_velocity.y * retain_factor + raw_velocity.y * smooth_factor;
                    spatial_source->smoothed_velocity.z = spatial_source->smoothed_velocity.z * retain_factor + raw_velocity.z * smooth_factor;

                    // 设置平滑后的矢量速度（OpenAL会自动计算多普勒效果）
                    spatial_source->source->SetVelocity(spatial_source->smoothed_velocity);

                    // 计算标量速度用于记录
                    spatial_source->movement_speed = math::Length(spatial_source->last_position, spatial_source->current_position) / time_diff;
                }
            }

            if(current_time>spatial_source->current_position_time)          // 更新时间和位置
            {
                spatial_source->last_position=spatial_source->current_position;
                spatial_source->last_position_time=spatial_source->current_position_time;
            }
        }

        // 方向性增益图：使用极坐标增益图计算方向性增益
        // 如果启用了方向性增益图（非全向），则计算并应用方向性增益
        if(listener && spatial_source->directional_pattern.IsEnabled())
        {
            const Vector3f &listener_pos = listener->GetPosition();

            // 计算从音源指向监听者的向量（归一化）
            Vector3f to_listener = listener_pos - spatial_source->current_position;
            float distance = math::Length(to_listener);
            if(distance > 0.0001f)  // 避免除以零
            {
                to_listener = to_listener / distance;  // 归一化

                // 计算方向性增益
                float directional_gain = spatial_source->directional_pattern.CalculateGain(spatial_source->direction, to_listener);

                // 应用方向性增益
                // 注意：这会覆盖 OpenAL 的锥形角度效果
                // 当使用极坐标增益图时，建议将 cone_angle 设置为 (360, 360) 以禁用 OpenAL 的锥形效果
                spatial_source->source->SetConeGain(directional_gain);
            }
        }

        // 频率相关衰减 + 场景级低通：根据距离与场景参数动态调整低通滤波器
        // 注意：如果 AudioSource 自己启用了滤波器，则场景级滤波不会覆盖它
        if(spatial_source->source && spatial_source->source->IsFilterEnabled())
        {
            if(spatial_source->lowpass_filter != 0)
            {
                if(alSourcei)
                    alSourcei(spatial_source->source->GetIndex(), AL_DIRECT_FILTER, AL_FILTER_NULL);

                if(alDeleteFilters)
                    alDeleteFilters(1, &spatial_source->lowpass_filter);

                spatial_source->lowpass_filter = 0;
                spatial_source->last_filter_gain = -1.0f;
                spatial_source->last_filter_gain_hf = -1.0f;
            }
        }
        else
        {
            const bool enable_scene_lowpass = scene_lowpass_enabled;
            const bool enable_distance_lowpass = frequency_dependent_attenuation;

            if((enable_scene_lowpass || enable_distance_lowpass) && listener && alGenFilters)
            {
                const Vector3f &listener_pos = listener->GetPosition();
                float distance = math::Length(listener_pos, spatial_source->current_position);

                // 计算距离因子（0=近距离，1=最大距离）
                float distance_factor = 0.0f;
                if(enable_distance_lowpass && spatial_source->max_distance > spatial_source->ref_distance)
                {
                    distance_factor = std::clamp((distance - spatial_source->ref_distance) / (spatial_source->max_distance - spatial_source->ref_distance), 0.0f, 1.0f);
                }

                // 远距离时降低高频增益，模拟空气吸收
                float distance_gain_hf = 1.0f;
                if(enable_distance_lowpass)
                {
                    distance_gain_hf = 1.0f - distance_factor * (1.0f - freq_atten_min_gain_hf);
                }

                float final_gain = enable_scene_lowpass ? scene_lowpass_gain : 1.0f;
                float final_gain_hf = distance_gain_hf * (enable_scene_lowpass ? scene_lowpass_gain_hf : 1.0f);

                final_gain = std::clamp(final_gain, 0.0f, 1.0f);
                final_gain_hf = std::clamp(final_gain_hf, 0.0f, 1.0f);

                // 只在参数变化显著时才更新滤波器（避免每帧都触发昂贵的OpenAL状态更新）
                if(std::abs(final_gain - spatial_source->last_filter_gain) > FREQ_ATTEN_CHANGE_THRESHOLD
                 || std::abs(final_gain_hf - spatial_source->last_filter_gain_hf) > FREQ_ATTEN_CHANGE_THRESHOLD)
                {
                    // 创建per-source滤波器（如果尚未创建）
                    if(spatial_source->lowpass_filter == 0)
                    {
                        alGetError();  // 清除之前的错误
                        alGenFilters(1, &spatial_source->lowpass_filter);
                        if(alGetError() != AL_NO_ERROR)
                        {
                            spatial_source->lowpass_filter = 0;  // 创建失败，确保ID为0
                            return(true);  // 继续执行，只是没有滤波器效果
                        }

                        if(alFilteri)
                        {
                            alFilteri(spatial_source->lowpass_filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
                            if(alGetError() != AL_NO_ERROR)
                            {
                                // 设置失败，清理并返回
                                if(alDeleteFilters)
                                    alDeleteFilters(1, &spatial_source->lowpass_filter);
                                spatial_source->lowpass_filter = 0;
                                return(true);
                            }
                        }
                    }

                    // 设置低通滤波器参数
                    if(spatial_source->lowpass_filter != 0 && alFilterf)
                    {
                        alGetError();  // 清除之前的错误

                        // 尝试设置两个参数，任一失败都放弃更新
                        alFilterf(spatial_source->lowpass_filter, AL_LOWPASS_GAIN, final_gain);
                        bool gain_ok = (alGetError() == AL_NO_ERROR);

                        alFilterf(spatial_source->lowpass_filter, AL_LOWPASS_GAINHF, final_gain_hf);
                        bool gainhf_ok = (alGetError() == AL_NO_ERROR);

                        // 只有两个参数都设置成功才应用滤波器
                        if(gain_ok && gainhf_ok && alSourcei)
                        {
                            alSourcei(spatial_source->source->GetIndex(), AL_DIRECT_FILTER, spatial_source->lowpass_filter);

                            if(alGetError() == AL_NO_ERROR)
                            {
                                spatial_source->last_filter_gain = final_gain;
                                spatial_source->last_filter_gain_hf = final_gain_hf;
                            }
                            // 如果应用失败，不更新缓存，下次会重试
                        }
                        // 如果参数设置失败，不更新缓存，下次会重试
                    }
                }
            }
            else if(spatial_source->lowpass_filter != 0)
            {
                if(alSourcei)
                    alSourcei(spatial_source->source->GetIndex(), AL_DIRECT_FILTER, AL_FILTER_NULL);

                if(alDeleteFilters)
                    alDeleteFilters(1, &spatial_source->lowpass_filter);

                spatial_source->lowpass_filter = 0;
                spatial_source->last_filter_gain = -1.0f;
                spatial_source->last_filter_gain_hf = -1.0f;
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
            current_time=ct;
        else
            current_time=GetTimeSec();

        // 递增帧计数器（用于分层更新）
        update_frame_counter++;

        const Vector3f &listener_pos = listener->GetPosition();

        float new_gain;
        int hear_count=0;

        for(auto source : source_list)
        {
            SpatialAudioSource *ptr = source;  // 为了保持代码兼容性

            if(!ptr->should_play)
            {
                if(ptr->source)          // 还有绑定的音源
                    ToMute(ptr);

                continue;   // 不需要播放的音源
            }

            new_gain=OnCheckGain(ptr);

            if(new_gain<=0)                 // 听不到声音
            {
                if(ptr->last_gain>0)     // 之前可以听到
                    ToMute(ptr);
                else
                    OnContinuedMute(ptr);  // 之前就听不到
            }
            else
            {
                if(ptr->last_gain<=0)
                {
                    if(!ToHear(ptr))       // 之前没声，尝试转为播放
                        new_gain=0;         // 没有足够可用音源或已播放结束，仍然听不到
                    else
                        ptr->last_update_frame = update_frame_counter;  // 记录更新帧
                }
                else
                {
                    // 分层更新：根据综合重要性决定更新频率
                    // 使用实际可听增益（new_gain）而非原始增益，这更准确反映用户听到的音量
                    double importance = CalculateImportance(ptr, new_gain, listener_pos);
                    uint update_interval;

                    if(importance >= TIER1_THRESHOLD)
                        update_interval = TIER1_UPDATE_INTERVAL;  // 高重要性：每帧更新
                    else if(importance >= TIER2_THRESHOLD)
                        update_interval = TIER2_UPDATE_INTERVAL;  // 中等重要性：每2帧更新
                    else
                        update_interval = TIER3_UPDATE_INTERVAL;  // 低重要性：每5帧更新

                    // 检查是否需要在当前帧更新此音源（使用模运算避免溢出）
                    uint64 frames_since_update = (update_frame_counter >= ptr->last_update_frame)
                        ? (update_frame_counter - ptr->last_update_frame)
                        : (UINT64_MAX - ptr->last_update_frame + update_frame_counter + 1);  // 处理溢出

                    if (frames_since_update >= update_interval)
                    {
                        UpdateSource(ptr);     // 刷新音源处理
                        ptr->last_update_frame = update_frame_counter;
                    }

                    OnContinuedHear(ptr);  // 持续可听
                }
            }

            ptr->last_gain=new_gain;

            if(new_gain>0)
                ++hear_count;
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

        alEffectf(reverb_effect, AL_REVERB_DENSITY, preset.params.Density);
        alEffectf(reverb_effect, AL_REVERB_DIFFUSION, preset.params.Diffusion);
        alEffectf(reverb_effect, AL_REVERB_GAIN, preset.params.Gain);
        alEffectf(reverb_effect, AL_REVERB_GAINHF, preset.params.GainHF);
        alEffectf(reverb_effect, AL_REVERB_DECAY_TIME, preset.params.DecayTime);
        alEffectf(reverb_effect, AL_REVERB_DECAY_HFRATIO, preset.params.DecayHFRatio);
        alEffectf(reverb_effect, AL_REVERB_REFLECTIONS_GAIN, preset.params.ReflectionsGain);
        alEffectf(reverb_effect, AL_REVERB_REFLECTIONS_DELAY, preset.params.ReflectionsDelay);
        alEffectf(reverb_effect, AL_REVERB_LATE_REVERB_GAIN, preset.params.LateReverbGain);
        alEffectf(reverb_effect, AL_REVERB_LATE_REVERB_DELAY, preset.params.LateReverbDelay);
        alEffectf(reverb_effect, AL_REVERB_AIR_ABSORPTION_GAINHF, preset.params.AirAbsorptionGainHF);
        alEffectf(reverb_effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, preset.params.RoomRolloffFactor);
        alEffecti(reverb_effect, AL_REVERB_DECAY_HFLIMIT, preset.params.DecayHFLimit);
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

    /**
     * 初始化频率相关衰减系统
     * 不再需要创建全局滤波器，每个音源会按需创建独立的滤波器
     */
    bool SpatialAudioWorld::InitFrequencyAttenuation()
    {
        scene_mutex.Lock();

        if(!alGenFilters)
        {
            scene_mutex.Unlock();
            return false;  // EFX 滤波器不可用
        }

        frequency_dependent_attenuation = true;

        scene_mutex.Unlock();
        return true;
    }

    /**
     * 关闭频率相关衰减系统
     * 立即清理所有音源的滤波器
     */
    void SpatialAudioWorld::CloseFrequencyAttenuation()
    {
        scene_mutex.Lock();

        frequency_dependent_attenuation = false;

        if(!scene_lowpass_enabled)
        {
            // 清理所有音源的滤波器
            for(auto source : source_list)
            {
                if(source && source->lowpass_filter != 0)
                {
                    if(alDeleteFilters)
                        alDeleteFilters(1, &source->lowpass_filter);
                    source->lowpass_filter = 0;
                }
            }
        }
        else
        {
            // 保留滤波器，强制下一帧重新应用场景低通参数
            for(auto source : source_list)
            {
                if(source)
                {
                    source->last_filter_gain = -1.0f;
                    source->last_filter_gain_hf = -1.0f;
                }
            }
        }

        scene_mutex.Unlock();
    }

    /**
     * 启用/禁用频率相关衰减
     */
    bool SpatialAudioWorld::EnableFrequencyAttenuation(bool enable)
    {
        if(enable && !alGenFilters)
        {
            return false;  // EFX 不可用
        }

        scene_mutex.Lock();

        // 如果禁用，按需清理或重置所有现有滤波器
        if(!enable && frequency_dependent_attenuation)
        {
            if(!scene_lowpass_enabled)
            {
                for(auto source : source_list)
                {
                    if(source && source->lowpass_filter != 0)
                    {
                        if(alDeleteFilters)
                            alDeleteFilters(1, &source->lowpass_filter);
                        source->lowpass_filter = 0;
                    }
                }
            }
            else
            {
                for(auto source : source_list)
                {
                    if(source)
                    {
                        source->last_filter_gain = -1.0f;
                        source->last_filter_gain_hf = -1.0f;
                    }
                }
            }
        }

        frequency_dependent_attenuation = enable;
        scene_mutex.Unlock();

        return true;
    }

    /**
     * 设置频率相关衰减参数
     */
    bool SpatialAudioWorld::SetFrequencyAttenuation(const FrequencyAttenuationConfig &config)
    {
        if(!config.enable)
        {
            EnableFrequencyAttenuation(false);
            return true;
        }

        if(!alGenFilters)
        {
            return false;  // EFX 不可用
        }

        scene_mutex.Lock();

        frequency_dependent_attenuation = true;
        freq_atten_min_gain_hf = std::clamp(config.min_gain_hf, 0.0f, 1.0f);

        for(auto source : source_list)
        {
            if(source)
            {
                source->last_filter_gain = -1.0f;
                source->last_filter_gain_hf = -1.0f;
            }
        }

        scene_mutex.Unlock();
        return true;
    }

    /**
     * 启用/禁用场景级低通
     */
    bool SpatialAudioWorld::EnableSceneLowpass(bool enable)
    {
        if(enable && !alGenFilters)
        {
            return false;  // EFX 不可用
        }

        scene_mutex.Lock();

        if(!enable && scene_lowpass_enabled)
        {
            if(!frequency_dependent_attenuation)
            {
                for(auto source : source_list)
                {
                    if(source && source->lowpass_filter != 0)
                    {
                        if(alDeleteFilters)
                            alDeleteFilters(1, &source->lowpass_filter);
                        source->lowpass_filter = 0;
                    }
                }
            }
            else
            {
                for(auto source : source_list)
                {
                    if(source)
                    {
                        source->last_filter_gain = -1.0f;
                        source->last_filter_gain_hf = -1.0f;
                    }
                }
            }
        }

        scene_lowpass_enabled = enable;
        scene_mutex.Unlock();

        return true;
    }

    /**
     * 设置场景级低通参数
     */
    bool SpatialAudioWorld::SetSceneLowpass(const float gain,const float gain_hf)
    {
        if(!alGenFilters)
        {
            return false;  // EFX 不可用
        }

        scene_mutex.Lock();

        scene_lowpass_gain = std::clamp(gain, 0.0f, 1.0f);
        scene_lowpass_gain_hf = std::clamp(gain_hf, 0.0f, 1.0f);
        scene_lowpass_enabled = true;

        for(auto source : source_list)
        {
            if(source)
            {
                source->last_filter_gain = -1.0f;
                source->last_filter_gain_hf = -1.0f;
            }
        }

        scene_mutex.Unlock();
        return true;
    }

    /**
     * 设置场景级低通参数(结构体版本)
     */
    bool SpatialAudioWorld::SetSceneLowpass(const SceneLowpassConfig &config)
    {
        if(!config.enable)
        {
            DisableSceneLowpass();
            return true;
        }

        return SetSceneLowpass(config.gain, config.gain_hf);
    }

    /**
     * 禁用场景级低通
     */
    void SpatialAudioWorld::DisableSceneLowpass()
    {
        EnableSceneLowpass(false);
    }

    /**
     * 设置音源的方向性增益图
     */
    void SpatialAudioWorld::SetDirectionalPattern(SpatialAudioSource *spatial_source, GainPatternType pattern_type)
    {
        if (!spatial_source)
            return;

        scene_mutex.Lock();
        spatial_source->directional_pattern.SetPattern(pattern_type);
        scene_mutex.Unlock();
    }

    /**
     * 设置音源的自定义方向性增益图
     */
    void SpatialAudioWorld::SetCustomDirectionalPattern(SpatialAudioSource *spatial_source, const PolarGainSample *samples, int count)
    {
        if (!spatial_source || !samples || count <= 0)
            return;

        scene_mutex.Lock();
        spatial_source->directional_pattern.SetCustomPattern(samples, count);
        scene_mutex.Unlock();
    }
}//namespace hgl::audio
