#pragma once

#include<hgl/platform/Platform.h>
#include<hgl/type/EnumUtil.h>

namespace hgl
{
    /**
    * 音频文件格式
    */
    enum class AudioFileType
    {
        None=0,          ///<起始定义，如使用表示自动根据扩展名识别

        Wav,             ///<Wav音波文件
        Vorbis,          ///<Vorbis OGG文件
        Opus,            ///<Opus OGG文件
        MIDI,            ///<MIDI音频文件

        ENUM_CLASS_RANGE(Wav,MIDI)
    };//enum AudioFileType

    AudioFileType CheckAudioExtName(const os_char *ext_name);
    AudioFileType CheckAudioFileType(const os_char *filename);
}//namespace hgl
