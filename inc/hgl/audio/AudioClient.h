#pragma once
/*
* CMAudio 客户端 C API（T6，DLL 模式）
*
* 纯 C 接口：客户端只需链接 CMP.AudioClient.dll + 本头文件，
* 不依赖任何 CMAudio/CMCore C++ 头。跨编译器/跨语言安全。
*
* 线程模型：
* - 引擎在独立线程运行（AudioEngineThread），与调用方隔离
* - 所有发送函数非阻塞（队列满返回 false）
* - 状态回传通过 AudioClient_PollResult 轮询
*
* 事件参数约定：
* - Play/AddCue/Snapshot 等名称参数传 UTF-8 字符串，内部按 FNV-1a 哈希
* - SetParam 的 param_name 同样传 UTF-8
* - bus 使用 AudioClientBus 枚举值
*/

#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
    #define AUDIO_API __declspec(dllexport)
#else
    #define AUDIO_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 总线分组（与 hgl::audio::AudioBusType 数值一致） */
enum AudioClientBus
{
    AudioClientBus_Master=0,
    AudioClientBus_Music,
    AudioClientBus_SFX,
    AudioClientBus_Ambient,
    AudioClientBus_UI
};

/* 状态回传（POD，与 hgl::audio::AudioEventResult 布局一致） */
typedef struct AudioClientResult
{
    uint32_t type;          /* AudioEventResultType：0=PlayStarted 1=PlayFinished 2=Stopped 3=LoadComplete 4=Error */
    uint32_t instance_id;   /* 实例 ID */
    uint32_t error_code;    /* 0=成功；1=未知Cue 2=无文件 3=加载失败 */
    uint32_t seq;           /* 请求序号（对账用） */
} AudioClientResult;

typedef struct AudioClient AudioClient;

/* ---- 生命周期 ---- */

/* 创建客户端（引擎未启动，可先注册 Cue）；queue_capacity=0 用默认 1024 */
AUDIO_API AudioClient *AudioClient_Create(uint32_t queue_capacity);

/* 销毁客户端（含停止引擎线程） */
AUDIO_API void AudioClient_Destroy(AudioClient *client);

/* 启动引擎线程；返回是否成功 */
AUDIO_API bool AudioClient_Start(AudioClient *client);

/* 停止引擎线程（阻塞等待退出） */
AUDIO_API void AudioClient_Stop(AudioClient *client);

/* ---- Cue 注册（Start 前调用）---- */

/* 注册单文件 Cue（name/file 均为 UTF-8） */
AUDIO_API bool AudioClient_AddCue(AudioClient *client,const char *name_utf8,const char *file_utf8);

/* 加载 TOML 配置（事件表 + 快照），path 为 UTF-8 */
AUDIO_API bool AudioClient_LoadCues(AudioClient *client,const char *toml_path_utf8);

/* ---- 事件发送（非阻塞）---- */

/* 触发 Cue 播放；成功返回 true 并写入 instance_id */
AUDIO_API bool AudioClient_Play(AudioClient *client,const char *cue_name_utf8,uint32_t seq,uint32_t *out_instance);

/* 停止指定实例 */
AUDIO_API bool AudioClient_StopInstance(AudioClient *client,uint32_t instance_id,uint32_t seq);

/* RTPC：设置实例实时参数（param_name UTF-8，经 Cue 的 rtpc 表映射） */
AUDIO_API bool AudioClient_SetParam(AudioClient *client,uint32_t instance_id,const char *param_name_utf8,float value,uint32_t seq);

/* 总线增益（bus 用 AudioClientBus） */
AUDIO_API bool AudioClient_SetBusVolume(AudioClient *client,int bus,float gain,uint32_t seq);

/* 总线静音 */
AUDIO_API bool AudioClient_SetBusMute(AudioClient *client,int bus,bool mute,uint32_t seq);

/* 应用混音快照（快照名 UTF-8） */
AUDIO_API bool AudioClient_Snapshot(AudioClient *client,const char *snapshot_name_utf8,uint32_t seq);

/* 全部实例暂停/恢复 */
AUDIO_API bool AudioClient_PauseAll(AudioClient *client,uint32_t seq);
AUDIO_API bool AudioClient_ResumeAll(AudioClient *client,uint32_t seq);

/* ---- 状态查询 ---- */

/* 非阻塞取一个回传；无则返回 false */
AUDIO_API bool AudioClient_PollResult(AudioClient *client,AudioClientResult *out);

/* 已处理事件总数 */
AUDIO_API uint32_t AudioClient_GetProcessedCount(AudioClient *client);

/* 活跃播放实例数 */
AUDIO_API int AudioClient_GetActiveInstanceCount(AudioClient *client);

/* 等待队列排空且引擎空闲；timeout_ms=0 无限等；返回是否完成 */
AUDIO_API bool AudioClient_WaitIdle(AudioClient *client,uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
