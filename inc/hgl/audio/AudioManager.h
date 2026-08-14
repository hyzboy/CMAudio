#pragma once

#include<hgl/type/ManagedArray.h>

namespace hgl::audio
{
    class AudioSource;
    class AudioBuffer;
    class AudioBus;

    /**
    * 简单的音频播放管理，为一般应用的简单操作工具
    */
    class AudioManager
    {
        struct AudioItem
        {
            AudioSource *source;
            AudioBuffer *buffer;
            AudioBus    *bus;

            AudioItem();
            ~AudioItem();

            bool Check();
            void Play(const os_char *,float);
        };//struct AudioItem

        ManagedArray<AudioItem> items;

    public:

        /**
        * 本类构造函数
        * @param count 创建的音源数量,默认为8
        */
        AudioManager(int count=8);
        ~AudioManager();

        /**
        * 播放一个音效
        * @param filename 文件名
        * @param gain 音量，默认为1
        */
        bool Play(const os_char *filename,float gain=1);

        /**
        * 将池中所有音源挂载到指定总线
        * @param b 总线（nullptr 表示解除挂载）
        */
        void SetBus(AudioBus *b);
    };//class AudioManager
}//namespace hgl::audio
