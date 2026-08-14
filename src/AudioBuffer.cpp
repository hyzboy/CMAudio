#include<hgl/log/Log.h>
#include<hgl/type/Pair.h>
#include<hgl/audio/OpenAL.h>
#include<hgl/audio/AudioBuffer.h>
#include<hgl/io/FileInputStream.h>
#include<hgl/io/MemoryInputStream.h>
#include<hgl/plugin/PlugIn.h>
#include"AudioDecode.h"

using namespace openal;

namespace hgl::audio
{
    using namespace io;

    const os_char *GetAudioDecodeName(const AudioFileType file_type);

    double LoadAudioData(int buffer_id,AudioFileType file_type,void *memory,int memory_size,uint &sample_rate)
    {
        ALenum format;
        ALvoid *data;
        ALsizei size;
        ALsizei freq;
        ALboolean loop;

        const os_char *plugin_name=GetAudioDecodeName(file_type);

        if(!plugin_name)RETURN_ERROR(0);

        AudioPlugInInterface decode{};
        AudioFloatPlugInInterface decode_float{};

        if(!GetAudioInterface(plugin_name,&decode,&decode_float))
            RETURN_ERROR(0);

        const bool use_float_data=(IsSupportFloatAudioData()&&decode_float.Load);

        bool use_float=use_float_data;

        if(use_float_data)
        {
            decode_float.Load((ALbyte *)memory, memory_size, &format,(float **)&data, &size, &freq, &loop);

            // 浮点解码失败(例如多声道浮点格式不受OpenAL支持)时，回退到16位解码
            if(format==0||data==nullptr||size<=0)
            {
                use_float=false;
                decode.Load((ALbyte *)memory, memory_size, &format, &data, &size, &freq, &loop);
            }
        }
        else
        {
            decode.Load((ALbyte *)memory, memory_size, &format, &data, &size, &freq, &loop);
        }

        alLastError();

        alBufferData(buffer_id, format, data, size, freq);

        if(use_float)
            decode_float.Clear(format, data, size, freq);
        else
            decode.Clear(format, data, size, freq);

        if(alLastError())RETURN_ERROR(0);

        sample_rate=freq;

        return AudioDataTime(size,format,freq);
    }

    void AudioBuffer::InitPrivate()
    {
        loaded=false;
        duration=0;
        data_size=0;
    }

    AudioBuffer::AudioBuffer(void *data,int size,AudioFileType file_type)
    {
        InitPrivate();
        Load(data,size,file_type);
    }

    AudioBuffer::AudioBuffer(InputStream *str,int size,AudioFileType file_type)
    {
        InitPrivate();
        Load(str,size,file_type);
    }

    AudioBuffer::AudioBuffer(const os_char *filename,AudioFileType file_type)
    {
        InitPrivate();
        if(filename)Load(filename,file_type);
    }

//     AudioBuffer::AudioBuffer(HAC *hac,const os_char *filename,AudioFileType file_type)
//     {
//         InitPrivate();
//         Load(hac,filename,file_type);
//     }

    AudioBuffer::~AudioBuffer()
    {
        Clear();
    }

    /**
    * 直接设置音频数据
    * @param format 音频数据格式，可以为“AL_FORMAT_MONO8、AL_FORMAT_MONO16、AL_FORMAT_STEREO16”
    * @param data 数据指针
    * @param size 数据长度
    * @param freq 采样频率
    * @return 音频数据可播放时间
    */
    bool AudioBuffer::SetData(uint format, const void* data, uint size, uint freq )
    {
        if(!alGenBuffers)RETURN_FALSE;

        Clear();
        alGenBuffers(1,&buffer_id);

        if(alLastError())return(loaded=false);

        alBufferData(buffer_id, format, data, size, freq);

        if(alLastError())return(loaded=false);

        duration=AudioDataTime(size,format,freq);

        sample_rate=freq;

        data_size=size;

        return(loaded=true);
    }

    bool AudioBuffer::SetData(const AudioDataInfo &info,const void *data)
    {
        const ALenum format=openal::ToOpenALFormat(info);

        if(format==0)return(false);

        return SetData(format,data,info.data_size,info.sample_rate);
    }

    /**
    * 从内存中加载一个音频文件到当前缓冲区,仅支持OGG和WAV。注：由于这个函数会一次性将音频数据载入内存，所以较长的音乐请使用CreateAudioPlayer，以免占用太多的内存。
    * @param memory 要加载数据的内存
    * @param file_type 音频文件类型
    * @return 是否加载成功
    */
    bool AudioBuffer::Load(void *memory,int size,AudioFileType file_type)
    {
        if(!alGenBuffers)RETURN_FALSE;

        Clear();
        alGenBuffers(1,&buffer_id);

        if(alLastError())return(loaded=false);

        if(!RangeCheck(file_type))
        {
            LogError(OS_TEXT("Audio file type unknow! AudioFileType:")+OSString::numberOf((int)file_type));
            alDeleteBuffers(1,&buffer_id);
            RETURN_FALSE;
        }
        else
        {
            duration=LoadAudioData(buffer_id,file_type,memory,size,sample_rate);
            data_size=size;
        }

        if(duration==0)
        {
            alDeleteBuffers(1,&buffer_id);
            RETURN_FALSE;
        }

        return(loaded=true);
    }

    /**
    * 从流中加载一个音频文件到当前缓冲区,仅支持OGG和WAV。注：由于这个函数会一次性将音频数据载入内存，所以较长的音乐请使用CreateAudioPlayer，以免占用太多的内存。
    * @param in 要加载数据的流
    * @param file_type 音频文件类型
    * @return 是否加载成功
    */
    bool AudioBuffer::Load(InputStream *in,int size,AudioFileType file_type)
    {
        if(!alGenBuffers)RETURN_FALSE;
        if(!in)RETURN_FALSE;
        if(size<=0)RETURN_FALSE;

        if(!RangeCheck(file_type))
        {
            LogError(OS_TEXT("Audio file type unknow! AudioFileType:")+OSString::numberOf((int)file_type));
            loaded=false;
        }
        else
        {
            char *memory=new char[size];

            in->Read(memory,size);
            loaded=Load(memory,size,file_type);

            delete[] memory;
        }

        RETURN_BOOL(loaded);
    }

    /**
    * 加载一个音频文件到当前缓冲区，仅支持OGG和WAV。注：由于这个函数会一次性将音频数据载入内存，所以较长的音乐请使用CreateAudioPlayer，以免占用太多的内存。
    * @param filename 音频文件名称
    * @param file_type 音频文件类型
    * @return 加载是否成功
    */
    bool AudioBuffer::Load(const os_char *filename,AudioFileType file_type)
    {
        if(!alGenBuffers)RETURN_FALSE;

        file_type=CheckAudioFileType(filename);

        if(!RangeCheck(file_type))
        {
            LogError(OS_TEXT("Audio file type unknow! AudioFile: ")+OSString(filename));
            loaded=false;
            RETURN_FALSE;
        }

        OpenFileInputStream file_stream(filename);

        RETURN_BOOL(Load(file_stream,file_stream->Available(),file_type));
    }

//     /**
//     * 加载一个音频文件到当前缓冲区，仅支持OGG和WAV。注：由于这个函数会一次性将音频数据载入内存，所以较长的音乐请使用CreateAudioPlayer，以免占用太多的内存。
//     * @param filename 音频文件名称
//     * @param file_type 音频文件类型
//     * @return 加载是否成功
//     */
//     bool AudioBuffer::Load(HAC *hac,const os_char *filename,AudioFileType file_type)
//     {
//         if(!alGenBuffers)RETURN_FALSE;
//
//         os_char *ext;
//         InputStream *stream;
//         bool result;
//
//         ext=strrchr(filename,u'.');
//         LowerString(ext);
//
//         if(file_type<=aftNone||file_type>=aftEnd)
//         {
//             if(strcmp(ext,u".ogg")==0)file_type=aftOGG;else
//             if(strcmp(ext,u".wav")==0)file_type=aftWAV;else
//             {
//                 PutError(u"未知的音频文件类型！AudioFileType:%d",file_type);
//                 RETURN_FALSE;
//             }
//         }
//
//         stream=hac->LoadFile(filename);
//         if(stream)
//         {
//             result=Load(stream,file_type);
//             delete stream;
//
//             return(result);
//         }
//
//         RETURN_FALSE;
//     }

    void AudioBuffer::Clear()
    {
        if(!alDeleteBuffers)return;
        if(loaded)
        {
            alDeleteBuffers(1,&buffer_id);
            loaded=false;
            duration=0;
        }
    }
}//namespace hgl::audio
