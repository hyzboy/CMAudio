#include<hgl/audio/EventTransport.h>
#include<hgl/thread/Thread.h>
#include<hgl/time/Time.h>

using namespace hgl;
using namespace hgl::audio;

static const uint32 EventCount=20000;
static int failed=0;

static void Check(bool condition)
{
    if(!condition)
        ++failed;
}

class EventProducer:public Thread
{
    SameProcessQueue *queue;
    atom_bool *done;

public:

    uint32 sent_count=0;
    bool timed_out=false;

    EventProducer(SameProcessQueue *q,atom_bool *is_done):queue(q),done(is_done){}

    bool Execute()override
    {
        const double deadline=GetUptimeSec()+10.0;

        for(uint32 i=0;i<EventCount;i++)
        {
            const AudioEvent event(AudioEventType::Play,i+1,0,i);

            while(!queue->Send(event))
            {
                if(IsStopRequested()||GetUptimeSec()>deadline)
                {
                    timed_out=true;
                    done->store(true);
                    return(false);
                }

                SleepSecond(0.0001);
            }

            ++sent_count;
        }

        done->store(true);
        return(false);
    }
};

class EventConsumer:public Thread
{
    SameProcessQueue *queue;
    atom_bool *done;

public:

    uint32 received_count=0;
    uint64 received_sum=0;

    EventConsumer(SameProcessQueue *q,atom_bool *is_done):queue(q),done(is_done){}

    bool Execute()override
    {
        for(;;)
        {
            if(IsStopRequested())
                return(false);

            bool received=false;
            AudioEvent event;

            while(queue->Recv(event))
            {
                received=true;
                ++received_count;
                received_sum+=event.cue_id;
            }

            if(done->load()&&!received)
                return(false);

            if(!received)
                SleepSecond(0.0001);
        }
    }
};

int main()
{
    {
        SameProcessQueue queue(8);
        Check(queue.GetPendingCount()==0);

        AudioEvent event(AudioEventType::Play,0xDEADBEEF,0,1);
        event.params[0]=3.5f;
        Check(queue.Send(event));
        Check(queue.GetPendingCount()==1);

        AudioEvent event2(AudioEventType::Stop,0,7,2);
        Check(queue.Send(event2));
        Check(queue.GetPendingCount()==2);

        AudioEvent received;
        Check(queue.Recv(received));
        Check(received.type==uint32(AudioEventType::Play));
        Check(received.cue_id==0xDEADBEEF);
        Check(received.seq==1);
        Check(received.params[0]==3.5f);
        Check(queue.GetPendingCount()==1);
        Check(queue.Recv(received));
        Check(received.type==uint32(AudioEventType::Stop));
        Check(received.instance_id==7);
        Check(queue.GetPendingCount()==0);
        Check(!queue.Recv(received));
    }

    {
        SameProcessQueue queue(8);
        const AudioEvent event(AudioEventType::Play,1,0,0);

        bool all_sent=true;
        for(int i=0;i<8;i++)
            if(!queue.Send(event))
                all_sent=false;

        Check(all_sent);
        Check(!queue.Send(event));
        Check(!queue.Send(event));
        Check(queue.GetDroppedCount()==2);

        AudioEvent received;
        Check(queue.Recv(received));
        Check(queue.Send(event));
        Check(queue.GetDroppedCount()==2);
    }

    {
        SameProcessQueue queue(8);
        const AudioEventResult result(AudioEventResultType::PlayStarted,42,0,100);
        Check(queue.PostResult(result));
        Check(queue.GetResultCount()==1);

        AudioEventResult received;
        Check(queue.PollResult(received));
        Check(received.instance_id==42);
        Check(received.seq==100);
        Check(queue.GetResultCount()==0);
        Check(!queue.PollResult(received));
    }

    {
        SameProcessQueue queue(1024);
        atom_bool done{false};
        EventConsumer consumer(&queue,&done);
        EventProducer producer(&queue,&done);

        if(!consumer.Start())
        {
            Check(false);
            return(1);
        }

        if(!producer.Start())
        {
            done.store(true);
            consumer.Join();
            Check(false);
            return(1);
        }

        const bool producer_joined=producer.Join(12.0);
        if(!producer_joined)
            producer.WaitExit();

        const bool consumer_joined=consumer.Join(12.0);
        if(!consumer_joined)
            consumer.WaitExit();

        Check(producer_joined&&consumer_joined);
        Check(!producer.timed_out&&producer.sent_count==EventCount);
        Check(consumer.received_count==EventCount);

        const uint64 expected_sum=uint64(EventCount)*(EventCount+1)/2;
        Check(consumer.received_sum==expected_sum);
    }

    return failed?1:0;
}
