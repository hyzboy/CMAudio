#pragma once

#include<hgl/CoreType.h>
#include<hgl/audio/AudioEvent.h>
#include<hgl/concurrency/DuplexQueue.h>

namespace hgl::audio
{
    /**
     * @brief CN: 音频事件传输抽象（T3），隔离生产者与音频引擎的部署差异。
     *        EN: Audio-event transport abstraction (T3), isolating deployment differences between producers and the audio engine.
     * @details CN: Send/Recv 构成“调用方 → 引擎”的请求通道；PostResult/PollResult
     *              构成“引擎 → 调用方”的响应通道。具体实现自行定义容量、阻塞和过载
     *              策略：同进程实现通常采用非阻塞有界 SPSC 队列，跨进程实现可因 OS
     *              管道背压而阻塞。调用方必须依据返回值处理队列满/空，不能假设所有实现
     *              都会缓存无限事件或在发送时等待空间。
     *          EN: Send/Recv form the caller-to-engine request channel; PostResult/PollResult
     *              form the engine-to-caller response channel. Implementations define
     *              capacity, blocking and overload policies. In-process implementations
     *              typically use bounded non-blocking SPSC queues; inter-process
     *              implementations may block under OS-pipe backpressure. Callers must
     *              handle full/empty results and must not assume unbounded buffering or
     *              that sends wait for available capacity.
     */
    class EventTransport
    {
    public:

        virtual ~EventTransport()=default;

        /** @brief CN: 发送事件；false 表示本次未接受，调用方可按策略重试或丢弃。 EN: Send an event; false means it was not accepted and may be retried or dropped by policy. */
        virtual bool Send(const AudioEvent &ev)=0;

        /** @brief CN: 引擎侧取出事件；false 表示当前无可读事件或传输失败。 EN: Receive an event on the engine side; false means no event is available or transport failed. */
        virtual bool Recv(AudioEvent &ev)=0;

        /** @brief CN: 引擎侧发送结果；false 表示本次回传未被接受。 EN: Post a result from the engine; false means the result was not accepted. */
        virtual bool PostResult(const AudioEventResult &r)=0;

        /** @brief CN: 调用方轮询并取出结果；false 表示当前无结果或读取失败。 EN: Poll for a result on the caller side; false means none is available or reading failed. */
        virtual bool PollResult(AudioEventResult &r)=0;

        /** @brief CN: 返回请求事件队列当前积压数。 EN: Return the current number of pending requests. */
        virtual int  GetPendingCount()const=0;

        /** @brief CN: 返回结果回传队列当前积压数。 EN: Return the current number of pending results. */
        virtual int  GetResultCount()const=0;

        /** @brief CN: 请求投递被拒绝的次数；后续重试成功也不撤销计数。 EN: Number of rejected request posts; later successful retries do not decrement it. */
        virtual uint64 GetDroppedCount()const=0;

        /** @brief CN: 回传投递被拒绝的次数；不支持该统计的实现返回 0。 EN: Number of rejected result posts; implementations without this statistic return 0. */
        virtual uint64 GetResultDroppedCount()const{return 0;}
    };//class EventTransport

    /**
     * @brief CN: 使用 CMCore 双向 SPSC 通道的同进程音频事件传输。
     *        EN: In-process audio-event transport using CMCore's bidirectional SPSC channel.
     * @details CN: 请求与结果各有独立的固定容量队列；每个方向都要求恰好一个生产者和
     *              一个消费者。容量是槽位数，队列满时 Send/PostResult 立即返回 false，
     *              并由对应 dropped 计数器记录拒绝次数；此实现不阻塞也不覆盖旧事件。
     *          EN: Requests and results use separate fixed-capacity queues, each requiring
     *              exactly one producer and one consumer. Capacity is measured in slots.
     *              Send/PostResult immediately returns false when full and increments
     *              the corresponding dropped counter; this implementation neither
     *              blocks nor overwrites older events.
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
