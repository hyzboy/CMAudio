#pragma once

#include<hgl/audio/AudioSource.h>

namespace hgl
{
    enum class AudioFilterPreset
    {
        None=0,         ///< 无滤波
        OldTelephone,   ///< 老式电话机
        TelephoneModern,///< 现代电话
        Intercom,       ///< 对讲机
        WalkieTalkie,   ///< 更窄对讲机
        Vinyl,          ///< 唱片
        Cassette,       ///< 磁带机
        OldRadio,       ///< 老式广播
        AMRadio,        ///< AM广播
        FMRadio,        ///< FM广播
        Megaphone,      ///< 手持扩音器
        PA,             ///< 公共广播
        Muffled,        ///< 闷声/隔墙
        InsideHelmet,   ///< 头盔内
        UnderwaterFresh,///< 淡水下
        UnderwaterSea   ///< 海水下
    };

    /**
     * 获取滤波预设配置
     * @param preset 预设类型
     * @return 对应的滤波参数
     */
    AudioFilterConfig GetAudioFilterPresetConfig(AudioFilterPreset preset);
}//namespace hgl
