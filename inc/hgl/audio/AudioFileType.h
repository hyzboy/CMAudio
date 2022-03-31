#ifndef HGL_AUDIO_FILE_TYPE_INCLUDE
#define HGL_AUDIO_FILE_TYPE_INCLUDE

#include<hgl/platform/Platform.h>
#include<hgl/TypeFunc.h>
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

        ENUM_CLASS_RANGE(Wav,Opus)
    };//enum AudioFileType

    AudioFileType CheckAudioExtName(const os_char *ext_name);
    AudioFileType CheckAudioFileType(const os_char *filename);
}//namespace hgl
#endif//HGL_AUDIO_FILE_TYPE_INCLUDE
