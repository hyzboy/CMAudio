#pragma once

#include<hgl/CoreType.h>
#include<hgl/audio/AudioEvent.h>
#include<hgl/concurrency/DuplexQueue.h>

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
    * 具体传输实现定义阻塞与过载策略。SameProcessQueue 使用非阻塞有界 SPSC，
    * IPCTransport 使用操作系统管道，可能因管道背压而阻塞。
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

        /** 请求队列 Push 被拒绝的次数（调用方重试后成功也会计入） */
        virtual uint64 GetDroppedCount()const=0;

        /** 回传队列 Push 被拒绝的次数；不支持该统计的传输实现返回 0 */
        virtual uint64 GetResultDroppedCount()const{return 0;}
    };//class EventTransport

    /**
    * 同进程事件传输（T3，静态库/DLL 模式）
    *
    * 使用 CMCore 通用双向 SPSC 队列（hgl::DuplexQueue）：
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
        hgl::DuplexQueue<AudioEvent,AudioEventResult> queue;

    public:

        SameProcessQueue(uint32 capacity=1024)
            :queue(capacity)
        {
        }

        // ---- EventTransport ----

        bool Send(const AudioEvent &ev)override
        {
            return queue.Send(ev);
        }

        bool Recv(AudioEvent &ev)override
        {
            return queue.Recv(ev);
        }

        bool PostResult(const AudioEventResult &r)override
        {
            return queue.SendResponse(r);
        }

        bool PollResult(AudioEventResult &r)override
        {
            return queue.RecvResponse(r);
        }

        int GetPendingCount()const override
        {
            return queue.GetRequestCount();
        }

        int GetResultCount()const override
        {
            return queue.GetResponseCount();
        }

        uint64 GetDroppedCount()const override
        {
            return queue.GetRequestDroppedCount();
        }

        uint64 GetResultDroppedCount()const override{return queue.GetResponseDroppedCount();}
    };//class SameProcessQueue
}//namespace hgl::audio
