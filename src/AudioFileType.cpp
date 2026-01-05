#include<hgl/audio/AudioFileType.h>
#include<hgl/type/StrChar.h>

namespace hgl
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
        OS_TEXT("Opus")
    };

    const os_char *GetAudioDecodeName(const AudioFileType aft)
    {
        if(!RangeCheck(aft))return(nullptr);

        return audio_decode_name[(size_t)aft-(size_t)AudioFileType::BEGIN_RANGE];
    }
}//namespace hgl
