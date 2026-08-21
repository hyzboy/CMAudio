#pragma once

#include<hgl/type/String.h>
#include<hgl/type/StringList.h>
#include<vector>

namespace hgl::audio
{
    /**
    * 音频总线分组类型（对应 AudioEngine 的标准子总线）
    */
    enum class AudioBusType
    {
        Master=0,        ///<主总线
        Music,           ///<音乐
        SFX,             ///<音效
        Ambient,         ///<环境
        UI               ///<界面
    };

    AudioBusType    AudioBusTypeFromString(const char *str);   ///< 由字符串解析总线分组（不识别返回 SFX）
    const char *    AudioBusTypeToString(AudioBusType type);   ///< 总线分组转字符串

    /**
    * RTPC 映射目标（实时参数可以驱动的播放属性）
    */
    enum class RTPCTarget
    {
        Pitch=0,        ///< 音高（倍频，0.5=降八度 2.0=升八度）
        Gain,           ///< 增益（线性）
        Lowpass,        ///< 低通截止频率（Hz）
        Pan,            ///< 声像（-1..1）
    };//enum class RTPCTarget

    RTPCTarget  RTPCTargetFromString(const char *str);          ///< 字符串解析 RTPC 目标（不识别返回 Pitch）
    const char *RTPCTargetToString(RTPCTarget target);          ///< RTPC 目标转字符串

    /**
    * 实时参数映射（RTPC，T2）
    * 游戏逻辑发 SetParam("rpm", v) → 引擎按本映射把 v 线性换算到目标属性。
    */
    struct RTPCConfig
    {
        OSString    param;          ///< 参数名（如 "rpm"）
        float       min=0.0f;       ///< 参数输入范围下限
        float       max=1.0f;       ///< 参数输入范围上限
        RTPCTarget  target=RTPCTarget::Pitch;   ///< 映射目标
        float       min_value=0.0f; ///< 目标范围下限
        float       max_value=1.0f; ///< 目标范围上限

        /**
        * 参数值 → 目标值（线性映射，输入 clamp 到 [min,max]）
        */
        float Map(float value)const
        {
            if(max<=min)
                return(min_value);

            float t=(value-min)/(max-min);
            if(t<0.0f)t=0.0f;else
            if(t>1.0f)t=1.0f;

            return(min_value+(max_value-min_value)*t);
        }
    };//struct RTPCConfig

    /**
    * 混音快照（T2）：一组总线增益调整（dB）
    * 进菜单/过场/战斗等场景切换时整体推拉各总线电平。
    */
    struct SnapshotConfig
    {
        float bus_gain[5];      ///< 各总线增益调整（dB，索引=AudioBusType 值）

        SnapshotConfig()
        {
            for(int i=0;i<5;i++)
                bus_gain[i]=0.0f;
        }

        float GetGain(AudioBusType bus)const{return bus_gain[int(bus)];}
        void  SetGain(AudioBusType bus,float db){bus_gain[int(bus)]=db;}
    };//struct SnapshotConfig

    /**
    * 声音事件配置（数据驱动层，P1-2）
    * 一个事件 = 一组音频文件变体 + 播放参数随机化 + 空间衰减 + 调度优先级 + 分组
    */
    struct SoundEventConfig
    {
        OSStringList files;                     ///< 音频文件变体列表（播放时随机选一个）

        OSStringList sequence;                  ///< 严格轮播文件列表（T2，与 files 互斥）
        OSStringList children;                  ///< 复合 Cue：并行触发的子 Cue 名列表（T2，与 files/sequence 互斥）

        float min_gain=1.0f;                    ///< 音量随机化下限
        float max_gain=1.0f;                    ///< 音量随机化上限
        float min_pitch=1.0f;                   ///< 音高随机化下限
        float max_pitch=1.0f;                   ///< 音高随机化上限

        float priority=1.0f;                    ///< 优先级（音源调度）
        float reference_distance=1.0f;          ///< 参考距离
        float max_distance=10000.0f;            ///< 最大距离
        float rolloff_factor=1.0f;              ///< 距离衰减系数

        bool  loop=false;                       ///< 是否循环
        AudioBusType bus_type=AudioBusType::SFX;///< 分组

        std::vector<RTPCConfig> rtpc;           ///< 实时参数映射表（T2）

        SoundEventConfig()=default;

        // OSStringList 禁用了拷贝构造（=delete），这里显式实现拷贝（用拷贝赋值做深拷贝）
        SoundEventConfig(const SoundEventConfig &other){CopyFrom(other);}

        SoundEventConfig &operator=(const SoundEventConfig &other)
        {
            if(this!=&other)
                CopyFrom(other);

            return *this;
        }

        float   RandomGain()const;              ///< 在 [min_gain,max_gain] 随机取音量
        float   RandomPitch()const;             ///< 在 [min_pitch,max_pitch] 随机取音高
        const OSString *RandomFile()const;      ///< 随机选一个文件变体；无文件返回 nullptr

        /**
        * 按序取轮播文件（T2）：第 index 次播放取 sequence[index % count]
        * 无轮播文件返回 nullptr
        */
        const OSString *SequenceFile(int index)const
        {
            const int count=sequence.GetCount();

            if(count<=0)
                return nullptr;

            return &sequence.GetString(index%count);
        }

    private:

        void CopyFrom(const SoundEventConfig &other)
        {
            files=other.files;                  // OSStringList 的深拷贝赋值
            sequence=other.sequence;
            children=other.children;
            min_gain=other.min_gain;
            max_gain=other.max_gain;
            min_pitch=other.min_pitch;
            max_pitch=other.max_pitch;
            priority=other.priority;
            reference_distance=other.reference_distance;
            max_distance=other.max_distance;
            rolloff_factor=other.rolloff_factor;
            loop=other.loop;
            bus_type=other.bus_type;
            rtpc=other.rtpc;
        }
    };//struct SoundEventConfig
}//namespace hgl::audio
