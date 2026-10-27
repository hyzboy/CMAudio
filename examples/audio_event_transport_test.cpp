#include<hgl/audio/EventTransport.h>

using namespace hgl;
using namespace hgl::audio;

static int failed=0;

static void Check(bool condition)
{
    if(!condition)
        ++failed;
}

int main()
{
    {
        SameProcessQueue queue(8);
        Check(queue.GetPendingCount()==0);

        AudioEvent event(AudioEventType::Play,0xDEADBEEF,0,1);
        event.params[0]=3.5f;
        Check(queue.Send(event));
        Check(queue.GetPendingCount()==1);

        AudioEvent stop_event(AudioEventType::Stop,0,7,2);
        Check(queue.Send(stop_event));
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

    return failed?1:0;
}
