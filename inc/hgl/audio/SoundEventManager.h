#pragma once

#include<hgl/type/String.h>
#include<hgl/type/UnorderedMap.h>
#include<hgl/audio/SoundEvent.h>

namespace hgl::audio
{
    /**
    * 声音事件管理器：事件名 → 配置映射 + TOML 加载（P1-2）
    *
    * - 数据驱动：音频内容策划在配置文件里，代码只按事件名触发
    * - 支持一个事件绑定多个音频文件变体（播放时随机化）
    */
    class SoundEventManager
    {
        UnorderedMap<OSString, SoundEventConfig> events;    ///< 事件名 → 配置

    public:

        SoundEventManager();
        ~SoundEventManager();

        bool AddEvent(const os_char *name,const SoundEventConfig &config);    ///< 添加/覆盖事件
        bool RemoveEvent(const os_char *name);                               ///< 删除事件
        const SoundEventConfig *GetEvent(const os_char *name)const;          ///< 按名查事件（未命中返回 nullptr）
        bool Contains(const os_char *name)const;                             ///< 是否存在该事件
        int  GetCount()const;                                                ///< 事件总数
        void Clear();                                                        ///< 清空全部事件

        /**
        * 从 TOML 文件加载声音事件配置（追加模式，不覆盖已有事件）
        * 格式示例见 examples/configs/sound_events.toml
        * @param filename TOML 配置文件路径（UTF-8 窄字符串，与 AudioMixerSceneConfig 一致）
        * @return 是否加载成功
        */
        bool LoadFromTOML(const char *filename);
    };//class SoundEventManager
}//namespace hgl::audio
