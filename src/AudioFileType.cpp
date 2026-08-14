#include<hgl/audio/AudioFileType.h>
#include<hgl/type/StrChar.h>
#include"AudioDecode.h"

namespace hgl::audio
{
    struct AudioFormatExt
    {
        os_char name[8];
        AudioFileType type;
    };//struct AudioFormatExt

    const AudioFormatExt audio_format_ext_name[]=
    {
        {OS_TEXT("wav"),    AudioFileType::Wav      },
        {OS_TEXT("ogg"),    AudioFileType::Vorbis   },
        {OS_TEXT("opus"),   AudioFileType::Opus     },
        {OS_TEXT("mid"),    AudioFileType::MIDI     },
        {OS_TEXT("midi"),   AudioFileType::MIDI     },
        {OS_TEXT(""),       AudioFileType::None     }
    };

    AudioFileType CheckAudioExtName(const os_char *ext_name)
    {
        auto *afp=audio_format_ext_name;

        while(afp->type!=AudioFileType::None)
        {
            if(hgl::strcmp(ext_name,afp->name)==0)
                return(afp->type);

            ++afp;
        }

        return(AudioFileType::None);
    }

    AudioFileType CheckAudioFileType(const os_char *filename)
    {
        const os_char *ext;
        os_char extname[16];

        ext=hgl::strrchr(filename,hgl::strlen(filename),'.');

        if(!ext)
            return(AudioFileType::None);

        ++ext;

        hgl::strcpy(extname,16,ext);
        hgl::to_lower_char(extname);

        return CheckAudioExtName(extname);
    }

    const os_char audio_decode_name[size_t(AudioFileType::RANGE_SIZE)][32]=
    {
        OS_TEXT("Wav"),
        OS_TEXT("Vorbis"),
        OS_TEXT("Opus"),
        OS_TEXT("MIDI")
    };

    const os_char *GetAudioDecodeName(const AudioFileType aft)
    {
        if(!RangeCheck(aft))return(nullptr);

        // 文件格式插件已通过 FileExtensions 能力上报扩展名，
        // 这里优先按规范扩展名走动态映射，硬编码表作为兜底。
        static const char *canonical_ext[]=
        {
            "wav",      // Wav
            "ogg",      // Vorbis
            "opus",     // Opus
            nullptr     // MIDI: 多个插件均上报 mid/midi，无唯一映射，保持旧行为
        };

        const size_t idx=(size_t)aft-(size_t)AudioFileType::BEGIN_RANGE;

        if(canonical_ext[idx])
        {
            const OSString *name=GetAudioPluginNameByExtension(canonical_ext[idx]);

            if(name)
                return name->c_str();
        }

        return audio_decode_name[idx];
    }
}//namespace hgl::audio
