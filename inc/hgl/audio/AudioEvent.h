#pragma once

#include<hgl/CoreType.h>
#include<cstring>

namespace hgl::audio
{
    /**
    * 事件指令类型（EVENT/CUE 机制，T1）
    * 调用方 → 音频引擎 的指令枚举（值固定，跨边界/跨版本稳定）
    */
    enum class AudioEventType : uint32
    {
        Play            = 0,    ///< 触发一个 Cue（cue_id 目标；params[0..2] 可选 3D 位置）
        Stop            = 1,    ///< 停止实例（instance_id；0=停止该 Cue 全部实例）
        SetParam        = 2,    ///< 实时参数 RTPC（params[0]=value；param 名走边带）
        SetBusVolume    = 3,    ///< 总线音量（params[0]=gain；bus 走边带）
        SetBusMute      = 4,    ///< 总线静音（params[0]=mute 0/1）
        LoadCue         = 5,    ///< 加载 Cue 包（pack 名走边带）
        UnloadCue       = 6,    ///< 卸载 Cue 包
        Snapshot        = 7,    ///< 切换混音快照（snapshot 名走边带）
        PauseAll        = 8,    ///< 全局暂停
        ResumeAll       = 9,    ///< 全局恢复
    };//enum class AudioEventType

    /**
    * 引擎状态回传类型（引擎 → 调用方）
    */
    enum class AudioEventResultType : uint32
    {
        PlayStarted     = 0,    ///< 实例已创建（回 instance_id）
        PlayFinished    = 1,    ///< 实例自然结束
        Stopped         = 2,    ///< 实例被 Stop / 被优先级抢占
        LoadComplete    = 3,    ///< Cue 包加载完成
        Error           = 4,    ///< 无效 Cue / 资源缺失 / 设备失败
    };//enum class AudioEventResultType

    /**
    * 事件指令（跨边界安全 POD，固定 48 字节）
    *
    * 纯值结构：无指针、无虚表、固定布局——可 memcpy 跨线程/跨进程。
    * Cue 名用 FNV-1a 哈希（CueNameHash）而非字符串：定长、可比较、跨进程安全。
    *
    * 布局：type(4) + cue_id(4) + instance_id(4) + params(32) + seq(4) = 48
    */
    struct AudioEvent
    {
        uint32  type;           ///< AudioEventType
        uint32  cue_id;         ///< Cue 名 FNV-1a 哈希（引擎侧查表；查不到记日志丢弃）
        uint32  instance_id;    ///< 目标实例（Play=0 表示新建；Stop/SetParam 定位实例）
        float   params[8];      ///< 定长参数块：pos[3] / rtpc value / gain / pan / ...
        uint32  seq;            ///< 发送端序号（调试/对账）

        AudioEvent()=default;

        AudioEvent(AudioEventType t,uint32 cue=0,uint32 inst=0,uint32 s=0)
        {
            type=uint32(t);
            cue_id=cue;
            instance_id=inst;
            seq=s;

            for(int i=0;i<8;i++)
                params[i]=0.0f;
        }

        /** 序列化：写入定长缓冲区（memcpy，返回是否成功） */
        bool ToBytes(void *out,int out_size)const
        {
            if(!out||out_size<(int)sizeof(AudioEvent))
                return(false);

            memcpy(out,this,sizeof(AudioEvent));
            return(true);
        }

        /** 反序列化：从定长缓冲区读取（memcpy，返回是否成功） */
        bool FromBytes(const void *in,int in_size)
        {
            if(!in||in_size<(int)sizeof(AudioEvent))
                return(false);

            memcpy(this,in,sizeof(AudioEvent));
            return(true);
        }
    };//struct AudioEvent

    static_assert(sizeof(AudioEvent)==48,"跨边界事件必须固定大小 48 字节");

    /**
    * 引擎状态回传（跨边界安全 POD，固定 16 字节）
    * 与发送通道同构的反向传输层载荷。
    */
    struct AudioEventResult
    {
        uint32  type;           ///< AudioEventResultType
        uint32  instance_id;    ///< 相关实例（0=全局）
        uint32  error_code;     ///< 0=OK
        uint32  seq;            ///< 对应事件的 seq（对账）

        AudioEventResult()=default;

        AudioEventResult(AudioEventResultType t,uint32 inst=0,uint32 err=0,uint32 s=0)
        {
            type=uint32(t);
            instance_id=inst;
            error_code=err;
            seq=s;
        }

        /** 序列化：写入定长缓冲区（memcpy，返回是否成功） */
        bool ToBytes(void *out,int out_size)const
        {
            if(!out||out_size<(int)sizeof(AudioEventResult))
                return(false);

            memcpy(out,this,sizeof(AudioEventResult));
            return(true);
        }

        /** 反序列化：从定长缓冲区读取（memcpy，返回是否成功） */
        bool FromBytes(const void *in,int in_size)
        {
            if(!in||in_size<(int)sizeof(AudioEventResult))
                return(false);

            memcpy(this,in,sizeof(AudioEventResult));
            return(true);
        }
    };//struct AudioEventResult

    static_assert(sizeof(AudioEventResult)==16,"跨边界回传必须固定大小 16 字节");

    /**
    * Cue 名 → 32 位哈希（FNV-1a）
    *
    * 标准 FNV-1a：跨平台/跨编译器确定性，自包含实现（不依赖 CMCore）。
    * 用于事件载荷中的 cue_id（定长、可比较、跨进程安全）。
    *
    * @param name Cue 名（UTF-8，如 "ui_click"）
    * @return 32 位 FNV-1a 哈希
    */
    uint32 CueNameHash(const char *name);
}//namespace hgl::audio
