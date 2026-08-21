#pragma once

#include<hgl/CoreType.h>
#include<hgl/thread/Thread.h>
#include<hgl/thread/Atomic.h>
#include<hgl/audio/EventTransport.h>
#include<hgl/audio/SoundEventManager.h>
#include<hgl/audio/AudioEngine.h>
#include<hgl/audio/AudioPlayer.h>
#include<vector>

namespace hgl::audio
{
    /**
    * 音频引擎线程（T4/T5）：事件驱动主循环
    *
    * 音频引擎隔离侧的核心：独立线程运行，主循环 =
    *   1. 批量消费事件队列（Send 的事件，一次清空，帧内一致）
    *   2. 分发执行（Play/Stop/SetParam/总线/快照 → SoundEventManager + AudioEngine + AudioPlayer）
    *   3. engine.Update(now)（驱动总线/资源/空间音频）
    *   4. 回传处理（PlayStarted/PlayFinished/Stopped/Error）
    *   5. SleepSecond(帧间隔)
    *
    * 线程隔离规则：
    * - OpenAL context 只在本线程创建/使用
    * - 调用方只碰 EventTransport（无锁队列），不共享任何引擎状态
    * - 状态查询走 WaitIdle + 原子计数
    *
    * 事件参数约定（T5）：
    * - Play：cue_id=Cue 名哈希
    * - Stop：instance_id=目标实例
    * - SetParam：instance_id=实例，cue_id=参数名哈希，params[0]=value
    * - SetBusVolume：params[0]=gain，params[1]=总线索引(AudioBusType)
    * - SetBusMute：params[0]=mute(0/1)，params[1]=总线索引
    * - Snapshot：cue_id=快照名哈希
    */
    class AudioEngineThread:public hgl::Thread
    {
        struct ActiveInstance
        {
            uint32      instance_id;    ///< 实例 ID（回传用）
            OSString    cue_name;       ///< Cue 名（RTPC 查表）
            AudioPlayer *player;        ///< 播放器
            bool        loop;           ///< 是否循环（播完清理判断）
        };

        EventTransport     *transport;      ///< 事件通道（外部持有）
        AudioEngine         engine;         ///< 音频引擎（总线/资源/空间音频）
        SoundEventManager   cues;           ///< Cue 表（事件名 → 配置）

        std::vector<ActiveInstance> instances;  ///< 活跃播放实例
        uint32 next_instance;                   ///< 实例 ID 分配器
        uint32 seq_counter;                     ///< sequence 轮播计数器

        atom<uint32> processed;             ///< 已处理事件计数（原子，供 WaitIdle 查询）
        atom<bool> busy;                    ///< 引擎线程正在处理事件（WaitIdle 用）
        atom<bool> running;                 ///< 引擎线程运行中

        double frame_interval;              ///< 帧间隔（秒，默认 10ms）

    protected:

        bool ProcStartThread()override;     ///< 线程启动：初始化（OpenAL context 归属本线程）
        bool Execute()override;             ///< 主循环体：事件消费 → 分发 → Update → 回传
        void ProcEndThread()override;       ///< 线程结束：清理

    public:

        AudioEngineThread(EventTransport *t);
        ~AudioEngineThread()override;

        bool DeletedAfterExit()const override{return false;}

        AudioEngine &GetEngine(){return engine;}
        SoundEventManager &GetCues(){return cues;}

        uint32 GetProcessedCount()const{return processed.load(std::memory_order_relaxed);}
        int    GetActiveInstanceCount()const{return (int)instances.size();}

        void SetFrameInterval(double sec){frame_interval=sec;}   ///< 帧间隔（默认 0.01）

        /**
        * 等待队列排空且本帧处理完成（测试/工具同步点）
        * @param timeout_ms 超时毫秒（0=无限等待）
        * @return 是否在超时前完成
        */
        bool WaitIdle(uint timeout_ms=0);

    private:

        AudioBus *GetBus(AudioBusType type);        ///< 总线类型 → 引擎总线
        void Dispatch(const AudioEvent &ev);    ///< 分发单个事件
        void ConsumeEvents();                   ///< 批量消费事件队列
        void FlushResults();                    ///< 处理回传（播完检测 → PlayFinished）
    };//class AudioEngineThread
}//namespace hgl::audio
