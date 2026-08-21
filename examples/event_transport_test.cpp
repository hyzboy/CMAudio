// Event Transport Test (T3)
// 验证传输层：SameProcessQueue 无锁 SPSC 环形队列
// 1) 单线程基本入出队  2) 队列满丢弃语义  3) 回传通道  4) 双线程并发无丢失无乱序
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <hgl/audio/EventTransport.h>

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
    std::cout << "== Event Transport Test (T3: SameProcessQueue) ==" << std::endl;

    // ---- 1. 单线程基本入出队 ----
    std::cout << "[1] 基本入出队" << std::endl;
    {
        SameProcessQueue q(8);

        Check("初始空", q.GetPendingCount()==0);

        AudioEvent ev(AudioEventType::Play, 0xDEADBEEF, 0, 1);
        ev.params[0]=3.5f;
        Check("Send 成功", q.Send(ev));
        Check("积压 1", q.GetPendingCount()==1);

        AudioEvent ev2(AudioEventType::Stop, 0, 7, 2);
        Check("Send 成功", q.Send(ev2));
        Check("积压 2", q.GetPendingCount()==2);

        AudioEvent got;
        Check("Recv 成功", q.Recv(got));
        Check("先出队的是第一个（FIFO）", got.type==uint32(AudioEventType::Play));
        Check("cue_id 一致", got.cue_id==0xDEADBEEF);
        Check("seq 一致", got.seq==1);
        Check("params[0] 一致", got.params[0]==3.5f);
        Check("积压 1", q.GetPendingCount()==1);

        Check("Recv 第二个", q.Recv(got));
        Check("第二个是 Stop", got.type==uint32(AudioEventType::Stop));
        Check("instance_id==7", got.instance_id==7);
        Check("空", q.GetPendingCount()==0);

        Check("空队列 Recv 返回 false", !q.Recv(got));
    }

    // ---- 2. 队列满丢弃 ----
    std::cout << "[2] 队列满丢弃" << std::endl;
    {
        SameProcessQueue q(8);      // 容量 8（<8 自动提升到 8）

        AudioEvent ev(AudioEventType::Play, 1, 0, 0);
        bool all8=true;
        for(int i=0;i<8;i++)
            if(!q.Send(ev))
                all8=false;
        Check("Send 8/8 全成功", all8);
        Check("第 9 个被丢弃（满）", !q.Send(ev));
        Check("第 10 个被丢弃（满）", !q.Send(ev));
        Check("丢弃计数 2", q.GetDroppedCount()==2);

        // 消费后腾出空间
        AudioEvent got;
        q.Recv(got);
        Check("腾出后 Send 成功", q.Send(ev));
        Check("丢弃计数仍 2", q.GetDroppedCount()==2);
    }

    // ---- 3. 回传通道 ----
    std::cout << "[3] 回传通道" << std::endl;
    {
        SameProcessQueue q(8);

        AudioEventResult r(AudioEventResultType::PlayStarted, 42, 0, 100);
        Check("PostResult 成功", q.PostResult(r));
        Check("回传积压 1", q.GetResultCount()==1);

        AudioEventResult got;
        Check("PollResult 成功", q.PollResult(got));
        Check("instance_id==42", got.instance_id==42);
        Check("seq==100", got.seq==100);
        Check("回传空", q.GetResultCount()==0);
        Check("空轮询返回 false", !q.PollResult(got));
    }

    // ---- 4. 双线程并发：生产者连续写、消费者连续读，无丢失无乱序 ----
    std::cout << "[4] 双线程并发（SPSC 无锁）" << std::endl;
    {
        SameProcessQueue q(1024);

        const int N = 100000;
        std::atomic<bool> done{false};
        std::atomic<uint32> recv_count{0};
        std::atomic<uint64> sum_recv{0};
        std::atomic<uint64> sum_expect{0};

        std::thread consumer([&]{
            AudioEvent got;
            uint32 count=0;
            uint64 sum=0;

            while(!done.load(std::memory_order_relaxed)||q.GetPendingCount()>0)
            {
                while(q.Recv(got))
                {
                    sum += got.cue_id;
                    ++count;
                }
            }

            recv_count.store(count);
            sum_recv.store(sum);
        });

        std::thread producer([&]{
            uint64 sum=0;

            for(int i=0;i<N;i++)
            {
                AudioEvent ev(AudioEventType::Play, (uint32)(i+1), 0, (uint32)i);
                while(!q.Send(ev)){}        // 等有空位（压力场景）
                sum += (uint32)(i+1);
            }

            sum_expect.store(sum);
            done.store(true,std::memory_order_relaxed);
        });

        producer.join();
        consumer.join();

        Check("100000 事件全部收到（无丢失）", recv_count.load()==(uint32)N);
        Check("cue_id 总和一致（无重复无遗漏）", sum_recv.load()==sum_expect.load());

        // 丢弃计数应为 0（消费足够快）
        std::cout << "    [INFO] dropped=" << q.GetDroppedCount() << std::endl;
    }

    std::cout << "== 结果: " << (failed ? "FAILED" : "ALL PASSED") << " (" << failed << " failures) ==" << std::endl;
    return failed ? 1 : 0;
}
