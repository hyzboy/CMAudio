#pragma once

#include<hgl/CoreType.h>
#include<hgl/thread/Thread.h>
#include<hgl/thread/Atomic.h>
#include<hgl/audio/EventTransport.h>
#include<hgl/audio/SoundEventManager.h>
#include<hgl/audio/AudioEngine.h>

namespace hgl::audio
{
    /**
    * 音频引擎线程（T4）：事件驱动主循环
    *
    * 音频引擎隔离侧的核心：独立线程运行，主循环 =
    *   1. 批量消费事件队列（Send 的事件，一次清空，帧内一致）
    *   2. 分发执行（Play/Stop/SetParam/总线/快照 → SoundEventManager + AudioEngine）
    *   3. engine.Update(now)（驱动总线/资源/空间音频）
    *   4. 处理回传（PostResult）
    *   5. SleepSecond(帧间隔)
    *
    * 线程隔离规则：
    * - OpenAL context 只在本线程创建/使用
    * - 调用方只碰 EventTransport（无锁队列），不共享任何引擎状态
    * - 状态查询走 WaitIdle + 原子快照
    *
    * 用法：
    *   SameProcessQueue q(1024);
    *   AudioEngineThread engine_thread(&q);
    *   engine_thread.Start();
    *   q.Send(ev);                       // 调用方发事件
    *   engine_thread.WaitIdle(1000);     // 测试/同步点：等队列排空+本帧处理完
    *   engine_thread.Stop();
    */
    class AudioEngineThread:public hgl::Thread
    {
        EventTransport     *transport;      ///< 事件通道（外部持有）
        AudioEngine         engine;         ///< 音频引擎（总线/资源/空间音频）
        SoundEventManager   cues;           ///< Cue 表（事件名 → 配置）

        uint32 next_instance;               ///< 实例 ID 分配器
        atom<uint32> processed;             ///< 已处理事件计数（原子，供 WaitIdle 查询）
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

        void SetFrameInterval(double sec){frame_interval=sec;}   ///< 帧间隔（默认 0.01）

        /**
        * 等待队列排空且本帧处理完成（测试/工具同步点）
        * @param timeout_ms 超时毫秒（0=无限等待）
        * @return 是否在超时前完成
        */
        bool WaitIdle(uint timeout_ms=0);

    private:

        void Dispatch(const AudioEvent &ev);    ///< 分发单个事件
        void ConsumeEvents();                   ///< 批量消费事件队列
        void FlushResults();                    ///< 处理回传
    };//class AudioEngineThread
}//namespace hgl::audio
