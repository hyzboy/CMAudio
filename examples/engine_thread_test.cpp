// Audio Engine Thread Test (T4)
// 验证引擎线程化：AudioEngineThread 独立线程主循环
// 事件消费 → 分发 → Update → 回传；WaitIdle 同步点
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <hgl/audio/AudioEngineThread.h>
#include <hgl/audio/EventTransport.h>
#include <hgl/time/Time.h>

using namespace hgl;
using namespace hgl::audio;

static int failed = 0;

static void Check(const char *name, bool cond)
{
    std::cout << (cond ? "  [PASS] " : "  [FAIL] ") << name << std::endl;
    if(!cond) ++failed;
}

int main()
{
    std::cout << "== Audio Engine Thread Test (T4: 引擎线程化) ==" << std::endl;

    // ---- 1. 启动/停止 + 线程隔离 ----
    std::cout << "[1] 启动/停止" << std::endl;
    {
        SameProcessQueue q(1024);
        AudioEngineThread t(&q);

        Check("Start 成功", t.Start());

        // 线程异步启动，稍候进入运行态
        hgl::SleepSecond(0.05);
        Check("引擎线程运行中", t.IsLive());
        Check("初始化后处理计数 0", t.GetProcessedCount()==0);

        t.WaitExit(1.0);
        Check("Stop 后线程退出", !t.IsLive());
    }

    // ---- 2. 事件消费：发 100 事件 → WaitIdle → 全部处理 ----
    std::cout << "[2] 事件消费与 WaitIdle" << std::endl;
    {
        SameProcessQueue q(1024);
        AudioEngineThread t(&q);
        Check("Start 成功", t.Start());

        // 发 100 个 Play 事件
        for(int i=0;i<100;i++)
        {
            AudioEvent ev(AudioEventType::Play, (uint32)(i+1), 0, (uint32)i);
            Check("Send 成功", q.Send(ev));
        }

        Check("队列积压 100", q.GetPendingCount()==100);

        Check("WaitIdle 完成", t.WaitIdle(3000));
        Check("100 事件全部处理", t.GetProcessedCount()==100);
        Check("队列已清空", q.GetPendingCount()==0);

        t.WaitExit(1.0);
    }

    // ---- 3. 回传：Play → PlayStarted（实例 ID 分配）----
    std::cout << "[3] 回传通道（PlayStarted）" << std::endl;
    {
        SameProcessQueue q(1024);
        AudioEngineThread t(&q);
        Check("Start 成功", t.Start());

        for(int i=0;i<10;i++)
        {
            AudioEvent ev(AudioEventType::Play, 0xABCD, 0, (uint32)i);
            q.Send(ev);
        }

        Check("WaitIdle 完成", t.WaitIdle(3000));

        // 收集回传（T5：未知 Cue → Error，不再回传 PlayStarted）
        AudioEventResult r;
        int errors=0;
        while(q.PollResult(r))
        {
            if(r.type==uint32(AudioEventResultType::Error))
                ++errors;
        }

        Check("10 个 Error 回传（未知 Cue）", errors==10);
        Check("回传队列已清空", q.GetResultCount()==0);

        t.WaitExit(1.0);
    }

    // ---- 4. 生产者在独立线程持续发事件（真实隔离场景）----
    std::cout << "[4] 多线程事件流（生产者线程 → 引擎线程）" << std::endl;
    {
        SameProcessQueue q(256);
        AudioEngineThread t(&q);
        Check("Start 成功", t.Start());

        const int N=5000;
        std::atomic<bool> done{false};

        std::thread producer([&]{
            for(int i=0;i<N;i++)
            {
                AudioEvent ev(AudioEventType::Play, (uint32)(i+1), 0, (uint32)i);
                while(!q.Send(ev)){}        // 等空位（压力）
            }
            done.store(true,std::memory_order_relaxed);
        });

        producer.join();
        Check("WaitIdle 完成（5000 事件）", t.WaitIdle(10000));
        Check("5000 事件全部处理", t.GetProcessedCount()==(uint32)N);
        Check("队列清空", q.GetPendingCount()==0);

        t.WaitExit(1.0);
    }

    std::cout << "== 结果: " << (failed ? "FAILED" : "ALL PASSED") << " (" << failed << " failures) ==" << std::endl;
    return failed ? 1 : 0;
}
