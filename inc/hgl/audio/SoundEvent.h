#pragma once

#include<hgl/type/String.h>
#include<hgl/type/StringList.h>

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
    * 声音事件配置（数据驱动层，P1-2）
    * 一个事件 = 一组音频文件变体 + 播放参数随机化 + 空间衰减 + 调度优先级 + 分组
    */
    struct SoundEventConfig
    {
        OSStringList files;                     ///< 音频文件变体列表（播放时随机选一个）

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

    private:

        void CopyFrom(const SoundEventConfig &other)
        {
            files=other.files;                  // OSStringList 的深拷贝赋值
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
        }
    };//struct SoundEventConfig
}//namespace hgl::audio
