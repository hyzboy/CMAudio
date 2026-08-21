#pragma once

#include<hgl/CoreType.h>
#include<hgl/audio/AudioEvent.h>
#include<hgl/thread/SpscQueue.h>

namespace hgl::audio
{
    /**
    * 事件传输层抽象（T3）
    *
    * 调用方 → 音频引擎 的事件通道与 引擎 → 调用方 的状态回传通道。
    * 三种部署形态实现同一接口：
    * - SameProcessQueue：同进程无锁 SPSC 环形队列（静态库/DLL 模式）
    * - IPCTransport：共享内存 + 命名管道（独占进程模式，T7）
    *
    * 语义约束（所有实现必须一致）：
    * - Send 永不阻塞（队列满则丢弃并计数，音频不能卡游戏）
    * - PollResult 非阻塞轮询
    */
    class EventTransport
    {
    public:

        virtual ~EventTransport()=default;

        /**
        * 发送一个事件（调用方 → 引擎）
        * @return true=入队成功；false=队列满被丢弃（GetDroppedCount 可查）
        */
        virtual bool Send(const AudioEvent &ev)=0;

        /**
        * 取回一个事件（引擎侧消费）
        * @return true=取到事件；false=队列空
        */
        virtual bool Recv(AudioEvent &ev)=0;

        /**
        * 回传状态（引擎 → 调用方）
        * @return true=入队成功；false=队列满被丢弃
        */
        virtual bool PostResult(const AudioEventResult &r)=0;

        /**
        * 取回一个状态回传（调用方侧轮询）
        * @return true=取到；false=队列空
        */
        virtual bool PollResult(AudioEventResult &r)=0;

        /** 事件队列当前积压数 */
        virtual int  GetPendingCount()const=0;

        /** 回传队列当前积压数 */
        virtual int  GetResultCount()const=0;

        /** 被丢弃的事件总数（队列满时） */
        virtual uint64 GetDroppedCount()const=0;
    };//class EventTransport

    /**
    * 同进程事件传输（T3，静态库/DLL 模式）
    *
    * 组合两个 CMCore 通用无锁 SPSC 队列（hgl::SpscQueue）：
    * - 事件通道：调用方线程 → 引擎线程（AudioEvent）
    * - 回传通道：引擎线程 → 调用方线程（AudioEventResult）
    *
    * 用法：
    *   SameProcessQueue q(1024);          // 容量（槽数）
    *   q.Send(ev);                        // 生产者
    *   AudioEvent got; q.Recv(got);       // 消费者
    */
    class SameProcessQueue : public EventTransport
    {
        hgl::SpscQueue<AudioEvent>       event_q;    ///< 事件通道（调用方→引擎）
        hgl::SpscQueue<AudioEventResult> result_q;   ///< 回传通道（引擎→调用方）

    public:

        SameProcessQueue(uint32 capacity=1024)
            :event_q(capacity),result_q(capacity)
        {
        }

        // ---- EventTransport ----

        bool Send(const AudioEvent &ev)override
        {
            return event_q.Push(ev);
        }

        bool Recv(AudioEvent &ev)override
        {
            return event_q.Pop(ev);
        }

        bool PostResult(const AudioEventResult &r)override
        {
            return result_q.Push(r);
        }

        bool PollResult(AudioEventResult &r)override
        {
            return result_q.Pop(r);
        }

        int GetPendingCount()const override
        {
            return event_q.GetCount();
        }

        int GetResultCount()const override
        {
            return result_q.GetCount();
        }

        uint64 GetDroppedCount()const override
        {
            return event_q.GetDroppedCount();
        }
    };//class SameProcessQueue
}//namespace hgl::audio
