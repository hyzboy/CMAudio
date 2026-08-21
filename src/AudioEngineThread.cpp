#include<hgl/audio/AudioEngineThread.h>
#include<hgl/audio/OpenAL.h>
#include<hgl/time/Time.h>

namespace hgl::audio
{
    AudioEngineThread::AudioEngineThread(EventTransport *t)
    {
        transport=t;
        next_instance=1;
        processed=0;
        running=false;
        frame_interval=0.01;
    }

    AudioEngineThread::~AudioEngineThread()
    {
        if(running.load(std::memory_order_relaxed))
            WaitExit(1.0);
    }

    bool AudioEngineThread::ProcStartThread()
    {
        // OpenAL context 归属本线程（隔离规则）；null 后端无设备也可测
        if(!openal::InitOpenAL(nullptr,"null",false,false))
            return(false);

        processed=0;
        running=true;

        return(true);
    }

    void AudioEngineThread::ProcEndThread()
    {
        running=false;

        openal::CloseOpenAL();
    }

    bool AudioEngineThread::Execute()
    {
        // 1. 批量消费事件（一次清空，帧内一致）
        ConsumeEvents();

        // 2. 驱动引擎
        engine.Update(GetTimeSec());

        // 3. 回传处理（如引擎侧有需要主动上报的状态）
        FlushResults();

        // 4. 帧间隔
        if(frame_interval>0)
            hgl::SleepSecond(frame_interval);

        return(running.load(std::memory_order_relaxed));
    }

    void AudioEngineThread::ConsumeEvents()
    {
        if(!transport)
            return;

        AudioEvent ev;

        while(transport->Recv(ev))
        {
            Dispatch(ev);
            processed.fetch_add(1,std::memory_order_relaxed);
        }
    }

    void AudioEngineThread::Dispatch(const AudioEvent &ev)
    {
        switch(AudioEventType(ev.type))
        {
            case AudioEventType::Play:
            {
                // 查 Cue 定义（无定义也分配实例并回传——T5 再接真实播放）
                const uint32 inst=next_instance++;
                const OSString *cue_name=nullptr;   // 由 cue_id 反查表（T5）

                (void)cue_name;

                if(transport)
                {
                    AudioEventResult r(AudioEventResultType::PlayStarted,inst,0,ev.seq);
                    transport->PostResult(r);
                }
                break;
            }

            case AudioEventType::Stop:
            {
                if(transport)
                {
                    AudioEventResult r(AudioEventResultType::Stopped,ev.instance_id,0,ev.seq);
                    transport->PostResult(r);
                }
                break;
            }

            default:
                // SetParam/SetBusVolume/SetBusMute/LoadCue/UnloadCue/Snapshot/PauseAll/ResumeAll
                // T5 接入：RTPC 映射、总线控制、快照切换、Cue 包加载
                break;
        }
    }

    void AudioEngineThread::FlushResults()
    {
        // T4：引擎侧暂无主动上报状态（播放完成检测在 T5 接入播放实例后）
    }

    bool AudioEngineThread::WaitIdle(uint timeout_ms)
    {
        if(!transport)
            return(true);

        const double start=GetTimeSec();

        // 阶段 1：等队列排空（生产者不再积压）
        while(transport->GetPendingCount()>0)
        {
            if(timeout_ms>0&&(GetTimeSec()-start)*1000.0>(double)timeout_ms)
                return(false);

            hgl::SleepSecond(0.001);
        }

        // 阶段 2：等引擎线程处理完最后一批
        // 取处理计数快照，睡一帧后再查：pending 仍为 0 且计数未变 → 处理完成
        for(;;)
        {
            const uint32 before=GetProcessedCount();

            if(timeout_ms>0&&(GetTimeSec()-start)*1000.0>(double)timeout_ms)
                return(false);

            hgl::SleepSecond(frame_interval*2.0);

            if(transport->GetPendingCount()==0&&GetProcessedCount()==before)
                return(true);

            // 有新事件到达或仍在处理，继续等
        }
    }
}//namespace hgl::audio
