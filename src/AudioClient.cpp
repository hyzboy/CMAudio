// AudioClient C API 实现（T6，DLL 模式）
// 包装 AudioEngineThread + SameProcessQueue，向客户端暴露纯 C 接口
#include<hgl/audio/AudioClient.h>

#include<hgl/audio/AudioEngineThread.h>
#include<hgl/audio/EventTransport.h>
#include<hgl/audio/SoundEventManager.h>
#include<hgl/utf.h>

using namespace hgl;
using namespace hgl::audio;

struct AudioClient
{
    SameProcessQueue  queue;
    AudioEngineThread engine;

    AudioClient(uint32 cap)
        :queue(cap?cap:1024),engine(&queue)
    {
    }

    ~AudioClient()
    {
        engine.WaitExit(2.0);
    }
};

static uint32 HashName(const char *name)
{
    return name?CueNameHash(name):0;
}

// ---- 生命周期 ----

AUDIO_API AudioClient *AudioClient_Create(uint32_t queue_capacity)
{
    return new AudioClient(queue_capacity);
}

AUDIO_API void AudioClient_Destroy(AudioClient *client)
{
    delete client;
}

AUDIO_API bool AudioClient_Start(AudioClient *client)
{
    if(!client)return false;

    return client->engine.Start();
}

AUDIO_API void AudioClient_Stop(AudioClient *client)
{
    if(!client)return;

    client->engine.WaitExit(2.0);
}

// ---- Cue 注册 ----

AUDIO_API bool AudioClient_AddCue(AudioClient *client,const char *name_utf8,const char *file_utf8)
{
    if(!client||!name_utf8||!file_utf8)return false;

    SoundEventConfig cfg;
    cfg.files.Add(ToOSString(file_utf8));

    return client->engine.GetCues().AddEvent(ToOSString(name_utf8).c_str(),cfg);
}

AUDIO_API bool AudioClient_LoadCues(AudioClient *client,const char *toml_path_utf8)
{
    if(!client||!toml_path_utf8)return false;

    return client->engine.GetCues().LoadFromTOML(toml_path_utf8);
}

// ---- 事件发送 ----

AUDIO_API bool AudioClient_Play(AudioClient *client,const char *cue_name_utf8,uint32_t seq,uint32_t *out_instance)
{
    if(!client||!cue_name_utf8)return false;

    AudioEvent ev(AudioEventType::Play,HashName(cue_name_utf8),0,seq);

    if(!client->queue.Send(ev))
        return false;

    if(out_instance)
        *out_instance=0;        // 实例 ID 由引擎分配，经回传 PlayStarted 取得

    return true;
}

AUDIO_API bool AudioClient_StopInstance(AudioClient *client,uint32_t instance_id,uint32_t seq)
{
    if(!client)return false;

    AudioEvent ev(AudioEventType::Stop,0,instance_id,seq);

    return client->queue.Send(ev);
}

AUDIO_API bool AudioClient_SetParam(AudioClient *client,uint32_t instance_id,const char *param_name_utf8,float value,uint32_t seq)
{
    if(!client||!param_name_utf8)return false;

    AudioEvent ev(AudioEventType::SetParam,HashName(param_name_utf8),instance_id,seq);
    ev.params[0]=value;

    return client->queue.Send(ev);
}

AUDIO_API bool AudioClient_SetBusVolume(AudioClient *client,int bus,float gain,uint32_t seq)
{
    if(!client)return false;

    AudioEvent ev(AudioEventType::SetBusVolume,0,0,seq);
    ev.params[0]=gain;
    ev.params[1]=(float)bus;

    return client->queue.Send(ev);
}

AUDIO_API bool AudioClient_SetBusMute(AudioClient *client,int bus,bool mute,uint32_t seq)
{
    if(!client)return false;

    AudioEvent ev(AudioEventType::SetBusMute,0,0,seq);
    ev.params[0]=mute?1.0f:0.0f;
    ev.params[1]=(float)bus;

    return client->queue.Send(ev);
}

AUDIO_API bool AudioClient_Snapshot(AudioClient *client,const char *snapshot_name_utf8,uint32_t seq)
{
    if(!client||!snapshot_name_utf8)return false;

    AudioEvent ev(AudioEventType::Snapshot,HashName(snapshot_name_utf8),0,seq);

    return client->queue.Send(ev);
}

AUDIO_API bool AudioClient_PauseAll(AudioClient *client,uint32_t seq)
{
    if(!client)return false;

    AudioEvent ev(AudioEventType::PauseAll,0,0,seq);

    return client->queue.Send(ev);
}

AUDIO_API bool AudioClient_ResumeAll(AudioClient *client,uint32_t seq)
{
    if(!client)return false;

    AudioEvent ev(AudioEventType::ResumeAll,0,0,seq);

    return client->queue.Send(ev);
}

// ---- 状态查询 ----

AUDIO_API bool AudioClient_PollResult(AudioClient *client,AudioClientResult *out)
{
    if(!client||!out)return false;

    AudioEventResult r;

    if(!client->queue.PollResult(r))
        return false;

    out->type=r.type;
    out->instance_id=r.instance_id;
    out->error_code=r.error_code;
    out->seq=r.seq;

    return true;
}

AUDIO_API uint32_t AudioClient_GetProcessedCount(AudioClient *client)
{
    if(!client)return 0;

    return client->engine.GetProcessedCount();
}

AUDIO_API int AudioClient_GetActiveInstanceCount(AudioClient *client)
{
    if(!client)return 0;

    return client->engine.GetActiveInstanceCount();
}

AUDIO_API bool AudioClient_WaitIdle(AudioClient *client,uint32_t timeout_ms)
{
    if(!client)return false;

    return client->engine.WaitIdle(timeout_ms);
}
