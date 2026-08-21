#include<hgl/audio/AudioEngineThread.h>
#include<hgl/audio/OpenAL.h>
#include<hgl/time/Time.h>

namespace hgl::audio
{
    AudioEngineThread::AudioEngineThread(EventTransport *t)
    {
        transport=t;
        next_instance=1;
        seq_counter=0;
        processed=0;
        busy=false;
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

    AudioBus *AudioEngineThread::GetBus(AudioBusType type)
    {
        switch(type)
        {
            case AudioBusType::Master:  return engine.GetMaster();
            case AudioBusType::Music:   return engine.GetMusic();
            case AudioBusType::Ambient: return engine.GetAmbient();
            case AudioBusType::UI:      return engine.GetUI();
            case AudioBusType::SFX:
            default:                    return engine.GetSFX();
        }
    }

    void AudioEngineThread::ConsumeEvents()
    {
        if(!transport)
            return;

        busy=true;

        AudioEvent ev;

        while(transport->Recv(ev))
        {
            Dispatch(ev);
            processed.fetch_add(1,std::memory_order_relaxed);
        }

        busy=false;
    }

    void AudioEngineThread::Dispatch(const AudioEvent &ev)
    {
        switch(AudioEventType(ev.type))
        {
            case AudioEventType::Play:
            {
                // 1. 查 Cue 定义
                const SoundEventConfig *cfg=cues.GetEventByHash(ev.cue_id);

                if(!cfg)
                {
                    if(transport)
                    {
                        AudioEventResult r(AudioEventResultType::Error,0,1,ev.seq);   // error_code=1 未知 Cue
                        transport->PostResult(r);
                    }
                    break;
                }

                // 2. 选文件：sequence 优先（轮播），否则 files 随机
                const OSString *file=nullptr;

                if(cfg->sequence.GetCount()>0)
                    file=cfg->SequenceFile((int)(seq_counter++));
                else
                    file=cfg->RandomFile();

                if(!file)
                {
                    if(transport)
                    {
                        AudioEventResult r(AudioEventResultType::Error,0,2,ev.seq);   // error_code=2 无文件
                        transport->PostResult(r);
                    }
                    break;
                }

                // 3. 创建播放器实例（引擎线程内，OpenAL context 归属本线程）
                AudioPlayer *player=new AudioPlayer;

                if(!player->Load(file->c_str()))
                {
                    delete player;

                    if(transport)
                    {
                        AudioEventResult r(AudioEventResultType::Error,0,3,ev.seq);   // error_code=3 加载失败
                        transport->PostResult(r);
                    }
                    break;
                }

                // 4. 应用 Cue 配置：随机增益/音高、循环、总线
                player->SetGain(cfg->RandomGain());
                player->SetPitch(cfg->RandomPitch());

                // 注意：Play(bool loop=true) 默认循环（BGM 设计），必须把 Cue 的 loop 传进去，
                // 否则 SetLoop(cfg->loop) 会被 Play() 内部覆盖
                player->Play(cfg->loop);

                switch(cfg->bus_type)
                {
                    case AudioBusType::Master:  player->SetBus(engine.GetMaster());  break;
                    case AudioBusType::Music:   player->SetBus(engine.GetMusic());   break;
                    case AudioBusType::Ambient: player->SetBus(engine.GetAmbient());break;
                    case AudioBusType::UI:      player->SetBus(engine.GetUI());      break;
                    case AudioBusType::SFX:
                    default:                    player->SetBus(engine.GetSFX());    break;
                }

                player->Play();

                // 5. 登记实例
                const uint32 inst=next_instance++;

                instances.push_back({inst,OSString(),player,cfg->loop});

                if(transport)
                {
                    AudioEventResult r(AudioEventResultType::PlayStarted,inst,0,ev.seq);
                    transport->PostResult(r);
                }
                break;
            }

            case AudioEventType::Stop:
            {
                // 按 instance_id 找实例
                for(auto it=instances.begin();it!=instances.end();++it)
                {
                    if(it->instance_id==ev.instance_id)
                    {
                        it->player->Stop();
                        it->player->WaitExit(1.0);
                        delete it->player;

                        if(transport)
                        {
                            AudioEventResult r(AudioEventResultType::Stopped,it->instance_id,0,ev.seq);
                            transport->PostResult(r);
                        }

                        instances.erase(it);
                        break;
                    }
                }
                break;
            }

            case AudioEventType::SetParam:
            {
                // RTPC：找实例 → 查 Cue 的 rtpc 表 → 应用映射
                for(const ActiveInstance &inst : instances)
                {
                    if(inst.instance_id!=ev.instance_id)
                        continue;

                    const SoundEventConfig *cfg=cues.GetEventByHash(ev.cue_id);

                    // 参数映射：遍历该 Cue 的 rtpc 表，匹配参数名哈希
                    // 简化：实例未记 cue 名，用事件里的 cue_id 直接匹配 Cue 的 rtpc 表
                    if(!cfg)
                        break;

                    for(const RTPCConfig &r : cfg->rtpc)
                    {
                        const float mapped=r.Map(ev.params[0]);

                        switch(r.target)
                        {
                            case RTPCTarget::Pitch:   inst.player->SetPitch(mapped);break;
                            case RTPCTarget::Gain:    inst.player->SetGain(mapped); break;
                            case RTPCTarget::Lowpass:
                            case RTPCTarget::Pan:
                            default: break;
                        }
                    }
                    break;
                }
                break;
            }

            case AudioEventType::SetBusVolume:
            {
                const AudioBusType bus=AudioBusType(int(ev.params[1]));
                AudioBus *b=GetBus(bus);

                if(b)
                    b->SetGain(ev.params[0]);
                break;
            }

            case AudioEventType::SetBusMute:
            {
                const AudioBusType bus=AudioBusType(int(ev.params[1]));
                AudioBus *b=GetBus(bus);

                if(b)
                    b->SetMute(ev.params[0]>0.5f);
                break;
            }

            case AudioEventType::Snapshot:
            {
                // 查快照 → 应用各总线增益（快照存 dB，转线性增益）
                const SnapshotConfig *snap=cues.GetSnapshotByHash(ev.cue_id);

                if(!snap)
                    break;

                for(int i=0;i<5;i++)
                {
                    AudioBus *b=GetBus(AudioBusType(i));

                    if(b)
                        b->SetGain(std::pow(10.0f,snap->bus_gain[i]/20.0f));
                }
                break;
            }

            case AudioEventType::PauseAll:
                for(ActiveInstance &inst : instances)
                    inst.player->Pause();
                break;

            case AudioEventType::ResumeAll:
                for(ActiveInstance &inst : instances)
                    inst.player->Resume();
                break;

            default:
                // LoadCue/UnloadCue：T5 之后接入 Cue 包管理
                break;
        }
    }

    void AudioEngineThread::FlushResults()
    {
        // 播完检测：非循环实例播放结束 → PlayFinished + 清理
        for(auto it=instances.begin();it!=instances.end();)
        {
            AudioPlayer *p=it->player;

            // 播完判定（双条件，null/无声后端 AL_STOPPED 不可靠）：
            // 1) 播放线程已退出（play_state==None，数据读尽）
            // 2) 播放时间已到总时长（数据已全部喂入 buffer 队列）
            const double played=p->GetPlayTime();
            const double total=p->GetTotalTime();

            const bool finished=!it->loop
                                &&(p->GetPlayState()==PlayState::None
                                   ||(total>0.0&&played>=total-0.05));

            if(finished)
            {
                if(transport)
                {
                    AudioEventResult r(AudioEventResultType::PlayFinished,it->instance_id,0,0);
                    transport->PostResult(r);
                }

                p->WaitExit(1.0);
                delete p;
                it=instances.erase(it);
            }
            else
            {
                ++it;
            }
        }
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

        // 阶段 2：等引擎线程空闲（busy=false 且处理计数稳定）
        for(;;)
        {
            const uint32 before=GetProcessedCount();

            if(timeout_ms>0&&(GetTimeSec()-start)*1000.0>(double)timeout_ms)
                return(false);

            hgl::SleepSecond(frame_interval*2.0);

            if(transport->GetPendingCount()==0&&!busy.load(std::memory_order_relaxed)&&GetProcessedCount()==before)
                return(true);

            // 有新事件到达或仍在处理，继续等
        }
    }
}//namespace hgl::audio
