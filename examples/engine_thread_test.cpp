#include<hgl/audio/AudioEngineThread.h>
#include<hgl/audio/EventTransport.h>
#include<hgl/log/Log.h>
#include<hgl/thread/Thread.h>
#include<hgl/time/Time.h>

using namespace hgl;
using namespace hgl::audio;

static int failed=0;
static int check_count=0;

static void Check(bool condition)
{
    ++check_count;
    if(!condition&&failed==0)
        failed=check_count;
}

static int StartEngine(AudioEngineThread &thread)
{
    if(!thread.Start())
        return(0);

    SleepSecond(0.05);
    return thread.IsRunning()?1:2;
}

class EventProducer:public Thread
{
    EventTransport *transport;
    uint32 event_count;

public:

    uint32 sent_count=0;
    bool timed_out=false;

    EventProducer(EventTransport *t,uint32 count):transport(t),event_count(count){}

    bool Execute()override
    {
        const double deadline=GetUptimeSec()+10.0;

        for(uint32 i=0;i<event_count;i++)
        {
            const AudioEvent event(AudioEventType::Play,i+1,0,i);

            while(!transport->Send(event))
            {
                if(IsStopRequested()||GetUptimeSec()>deadline)
                {
                    timed_out=true;
                    return(false);
                }

                SleepSecond(0.0001);
            }

            ++sent_count;
        }

        return(false);
    }
};

int main()
{
    {
        SameProcessQueue queue(1024);
        AudioEngineThread thread(&queue);
        const int startup=StartEngine(thread);

        if(startup==0)
        {
            thread.StopAndJoin();
            Check(false);
            return(failed);
        }

        if(startup==2)
        {
            thread.StopAndJoin();
            GLogWarning(U8_TEXT("Audio engine integration test skipped: OpenAL initialization failed"));
            return(77);
        }

        Check(true);

        Check(thread.GetProcessedCount()==0);

        Check(thread.StopAndJoin());
        Check(!thread.IsRunning());
    }

    {
        SameProcessQueue queue(1024);
        AudioEngineThread thread(&queue);
        const bool running=StartEngine(thread)==1;
        Check(running);

        if(running)
        {
            bool all_sent=true;
            for(uint32 i=0;i<100;i++)
            {
                const AudioEvent event(AudioEventType::Play,i+1,0,i);
                if(!queue.Send(event))
                    all_sent=false;
            }

            Check(all_sent);
            Check(queue.GetPendingCount()==100);
            Check(thread.WaitIdle(3000));
            Check(thread.GetProcessedCount()==100);
            Check(queue.GetPendingCount()==0);
        }

        Check(thread.StopAndJoin());
    }

    {
        SameProcessQueue queue(1024);
        AudioEngineThread thread(&queue);
        const bool running=StartEngine(thread)==1;
        Check(running);

        if(running)
        {
            for(uint32 i=0;i<10;i++)
            {
                const AudioEvent event(AudioEventType::Play,0xABCD,0,i);
                queue.Send(event);
            }

            Check(thread.WaitIdle(3000));

            AudioEventResult result;
            int errors=0;
            while(queue.PollResult(result))
                if(result.type==uint32(AudioEventResultType::Error))
                    ++errors;

            Check(errors==10);
            Check(queue.GetResultCount()==0);
        }

        Check(thread.StopAndJoin());
    }

    {
        SameProcessQueue queue(256);
        AudioEngineThread thread(&queue);
        const bool running=StartEngine(thread)==1;
        Check(running);

        if(running)
        {
            static const uint32 event_count=5000;
            EventProducer producer(&queue,event_count);

            if(producer.Start())
            {
                const bool producer_joined=producer.Join(12.0);
                Check(producer_joined);

                if(!producer_joined)
                    producer.StopAndJoin(2.0);

                Check(!producer.timed_out&&producer.sent_count==event_count);
                Check(thread.WaitIdle(10000));
                Check(thread.GetProcessedCount()==event_count);
                Check(queue.GetPendingCount()==0);
            }
            else
            {
                Check(false);
            }
        }

        Check(thread.StopAndJoin());
    }

    if(failed)
        GLogError(U8_TEXT("Audio engine thread test failed at check %d"),failed);

    if(!failed)
        GLogInfo(U8_TEXT("Audio engine thread test passed"));

    return failed;
}
