#pragma once

#include<hgl/math/Vector.h>
#include<hgl/type/Pool.h>
#include<hgl/type/Map.h>
#include<hgl/type/SortedSet.h>
#include<hgl/audio/ConeAngle.h>
#include<hgl/audio/ReverbPreset.h>
#include<hgl/thread/ThreadMutex.h>

namespace hgl
{   
    using namespace math;

    class AudioBuffer;
    class AudioSource;
    class AudioListener;

    /**
     * 音源配置参数结构体
     */
    struct SpatialAudioSourceConfig
    {
        AudioBuffer *   buffer                  = nullptr;          ///< 音频缓冲区
        Vector3f        position                = Vector3f(0,0,0);  ///< 初始位置
        float           gain                    = 1.0f;             ///< 音量增益
        uint            distance_model          = 0;                ///< 距离衰减模型
        float           rolloff_factor          = 1.0f;             ///< 环境衰减系数
        float           ref_distance            = 1.0f;             ///< 参考距离
        float           max_distance            = 10000.0f;         ///< 最大距离
        bool            loop                    = false;            ///< 是否循环播放
        float           doppler_factor          = 0.0f;             ///< 多普勒效果强度
        float           air_absorption_factor   = 0.0f;             ///< 空气吸收因子
    };

    /**
     * 逻辑发声源
     */
    struct SpatialAudioSource
    {
        friend const double GetGain(AudioListener *,SpatialAudioSource *);
        friend class SpatialAudioWorld;

    private:

        AudioBuffer *buffer;

    public:

        bool loop;                                          ///< 是否循环播放
        float gain;                                         ///< 音量增益

        uint distance_model;                                ///< 音量衰减模型
        float rolloff_factor;                               ///< 环境衰减系数，默认为1
        float doppler_factor;                               ///< 多普勒效果强度，默认为0
        float air_absorption_factor;                        ///< 空气吸收因子，默认为0

        float ref_distance;                                 ///< 参考距离
        float max_distance;                                 ///< 最大距离
        ConeAngle cone_angle;
        Vector3f velocity;
        Vector3f direction;

    private:

        double start_play_time;                             ///< 开始播放时间
        bool is_play;                                       ///< 是否需要播放

        Vector3f last_pos;
        double last_time;

        Vector3f cur_pos;
        double cur_time;

        double move_speed;

        double last_gain;                                   ///< 上一次的音量
        
        // 淡入淡出状态
        bool is_fading;                                     ///< 是否正在执行淡入淡出
        double fade_start_time;                             ///< 淡入淡出开始时间
        double fade_duration;                               ///< 淡入淡出持续时间
        double fade_start_gain;                             ///< 淡入淡出开始时的增益
        double fade_target_gain;                            ///< 淡入淡出目标增益

        AudioSource *source;

    public:

        /**
         * 构造函数：通过配置结构体初始化所有成员变量
         * @param config 音源配置参数结构体，包含所有必要的初始化参数
         */
        explicit SpatialAudioSource(const SpatialAudioSourceConfig &config)
            : buffer(config.buffer)
            , loop(config.loop)
            , gain(config.gain)
            , distance_model(config.distance_model)
            , rolloff_factor(config.rolloff_factor)
            , doppler_factor(config.doppler_factor)
            , air_absorption_factor(config.air_absorption_factor)
            , ref_distance(config.ref_distance)
            , max_distance(config.max_distance)
            , start_play_time(0)
            , is_play(false)
            , last_time(0)
            , cur_time(0)
            , move_speed(0)
            , last_gain(0)
            , is_fading(false)
            , fade_start_time(0)
            , fade_duration(0)
            , fade_start_gain(0)
            , fade_target_gain(0)
            , source(nullptr)
        {
            velocity = Vector3f(0, 0, 0);
            direction = Vector3f(0, 0, 0);
            last_pos = config.position;
            cur_pos = config.position;
        }

        /**
         * 请求播放这个音源
         * @param play_time 开始播放时间(默认为0，表示只有能听到时才开始播放)
         */
        void Play(const double play_time=0)
        {
            start_play_time=play_time;
            is_play=true;
            // 不重置 last_time，避免破坏 MoveTo() 的逻辑
        }

        /**
         * 请求立即停止这个音源
         */
        void Stop()
        {
            is_play=false;
        }

        /**
         * 将音源移动到指定位置
         * @param pos 坐标
         * @param ct 当前时间
         */
        void MoveTo(const Vector3f &pos,const double &ct)
        {
            last_pos=cur_pos;
            last_time=cur_time;

            cur_pos=pos;
            cur_time=ct;
        }
    };//struct SpatialAudioSource

    const double GetGain(AudioListener *,SpatialAudioSource *);                                     ///< 获取相对音量

    /**
     * 空间音频场景管理
     * 
     * 内存管理说明：
     * - SpatialAudioSource 对象由 Create() 使用 new 分配，由 Delete() 或 Clear() 释放
     * - AudioSource 对象通过 source_pool 对象池管理，自动复用
     * - 调用者负责管理 AudioBuffer 和 AudioListener 对象的生命周期
     * 
     * 线程安全说明：
     * - 所有公共 API 方法（Create、Delete、Clear、Update、SetListener、SetDistance、
     *   InitReverb、CloseReverb、SetReverbPreset、EnableReverb）都是线程安全的
     * - 多线程环境下可以安全地从不同线程调用这些方法
     * - SpatialAudioSource 对象的成员方法（Play、Stop、MoveTo）不是线程安全的，
     *   应该在持有适当锁的情况下调用，或确保单线程访问
     */
    class SpatialAudioWorld                                                                         ///< 空间音频场景管理
    {
    protected:

        double cur_time;                                                                            ///< 当前时间

        float ref_distance;                                                                         ///< 默认参考距离
        float max_distance;                                                                         ///< 默认最大距离

        AudioListener *listener;                                                                    ///< 监听者

        ObjectPool<AudioSource> source_pool;                                                        ///< 音源对象池
        SortedSet<SpatialAudioSource *> source_list;                                                ///< 音源列表
        
        ThreadMutex scene_mutex;                                                                    ///< 线程互斥锁
        
        uint aux_effect_slot;                                                                       ///< 辅助效果槽
        uint reverb_effect;                                                                         ///< 混响效果
        bool reverb_enabled;                                                                        ///< 混响是否启用

    protected:

        bool ToMute(SpatialAudioSource *);                                                          ///< 转为静音处理
        bool ToHear(SpatialAudioSource *);                                                          ///< 转为发声处理

        bool UpdateSource(SpatialAudioSource *);                                                    ///< 刷新音源处理
        
        void ApplyReverbPreset(const AudioReverbPresetProperties &);                                     ///< 应用混响预设

    public:     // 事件

        virtual float   OnCheckGain(SpatialAudioSource *asi)                                        ///< 检测音量事件
        {
            return asi?GetGain(listener,asi)*asi->gain:0;
        }

        virtual void    OnToMute(SpatialAudioSource *){/* 无任何处理，请自行重载处理 */}           ///< 从有声变为听不到声音
        virtual void    OnToHear(SpatialAudioSource *){/* 无任何处理，请自行重载处理 */}           ///< 从听不到声音变为能听到声音

        virtual void    OnContinuedMute(SpatialAudioSource *){/* 无任何处理，请自行重载处理 */}   ///< 持续听不到声音
        virtual void    OnContinuedHear(SpatialAudioSource *){/* 无任何处理，请自行重载处理 */}   ///< 持续可以听到声音

        virtual bool    OnStopped(SpatialAudioSource *){return true;}                               ///< 单个音源播放结束事件，返回 true 表示可以释放该音源，返回 false 则仍占用该音源

    public:

        SpatialAudioWorld(int max_source,AudioListener *al);                                        ///< 构造函数(指定最大音源数)
        virtual ~SpatialAudioWorld();                                                               ///< 析构函数

                void                SetListener(AudioListener *al)                                  ///< 设置监听者
                {
                    if(al)  // 仅在非空时设置
                    {
                        scene_mutex.Lock();
                        listener=al;
                        scene_mutex.Unlock();
                    }
                }

                /**
                 * 设定距离场参数，单位由调用方自行定义，后续坐标等数据需保持一致
                 * @param rd 参考距离
                 * @param md 最大距离
                 */
                void                SetDistance(const float &rd,const float &md)                    ///< 设定参考距离
                {
                    scene_mutex.Lock();
                    ref_distance=rd;
                    max_distance=md;
                    scene_mutex.Unlock();
                }

                bool                InitReverb();                                                   ///< 初始化混响系统
                void                CloseReverb();                                                  ///< 关闭混响系统
                bool                SetReverbPreset(AudioReverbPreset preset);                           ///< 设置混响预设(使用 OpenAL Soft 官方预设)
                bool                EnableReverb(bool enable);                                      ///< 启用/禁用混响

        virtual SpatialAudioSource *Create(const SpatialAudioSourceConfig &config);                 ///< 创建一个音源（通过配置结构体设置所有参数）
        virtual void                Delete(SpatialAudioSource *);                                   ///< 删除一个音源

        virtual void                Clear();                                                        ///< 清除所有音源

        virtual int                 Update(const double &ct=0);                                     ///< 刷新，返回仍在发声的音源数量
    };//class SpatialAudioWorld
}//namespace hgl
