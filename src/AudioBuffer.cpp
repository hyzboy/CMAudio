#include<hgl/log/Log.h>
#include<hgl/type/Pair.h>
#include<hgl/audio/OpenAL.h>
#include<hgl/audio/AudioBuffer.h>
#include<hgl/audio/AudioEQ.h>
#include<hgl/io/FileInputStream.h>
#include<hgl/io/MemoryInputStream.h>
#include<hgl/plugin/PlugIn.h>
#include"AudioDecode.h"

#include<vector>
#include<cstring>

using namespace openal;

namespace hgl::audio
{
    using namespace io;

    const os_char *GetAudioDecodeName(const AudioFileType file_type);

    DecodedAudio *DecodeAudio(AudioFileType file_type,void *memory,int memory_size)
    {
        const os_char *plugin_name=GetAudioDecodeName(file_type);

        if(!plugin_name)return nullptr;

        DecodedAudio *result=new DecodedAudio;

        if(!GetAudioInterface(plugin_name,&result->decode,&result->decode_float))
        {
            delete result;
            return nullptr;
        }

        const bool use_float_data=(IsSupportFloatAudioData()&&result->decode_float.Load);

        result->use_float=use_float_data;

        ALboolean loop;

        if(use_float_data)
        {
            result->decode_float.Load((ALbyte *)memory,memory_size,&result->format,(float **)&result->data,&result->size,&result->freq,&loop);

            // 浮点解码失败(例如多声道浮点格式不受OpenAL支持)时，回退到16位解码
            if(result->format==0||result->data==nullptr||result->size<=0)
            {
                result->use_float=false;
                result->decode.Load((ALbyte *)memory,memory_size,&result->format,&result->data,&result->size,&result->freq,&loop);
            }
        }
        else
        {
            result->decode.Load((ALbyte *)memory,memory_size,&result->format,&result->data,&result->size,&result->freq,&loop);
        }

        if(result->format==0||result->data==nullptr||result->size<=0)
        {
            result->Release();
            delete result;
            return nullptr;
        }

        result->duration=AudioDataTime(result->size,result->format,result->freq);

        return result;
    }

    void DecodedAudio::Release()
    {
        if(!data)return;

        if(use_float)
            decode_float.Clear(format,data,size,freq);
        else
            decode.Clear(format,data,size,freq);

        data=nullptr;
    }

    bool UploadDecoded(uint buffer_id,DecodedAudio *decoded)
    {
        if(!decoded)return false;

        alLastError();

        alBufferData(buffer_id,decoded->format,decoded->data,decoded->size,decoded->freq);

        decoded->Release();

        delete decoded;

        return !alLastError();
    }

    void AudioBuffer::InitPrivate()
    {
        loaded=false;
        duration=0;
        data_size=0;
        ref_count=0;
    }

    uint AudioBuffer::IncRef()
    {
        return ref_count.fetch_add(1)+1;
    }

    uint AudioBuffer::DecRef()
    {
        return ref_count.fetch_sub(1)-1;
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

        // 应用 buffer 级 EQ（P2）：data 为 const，需复制后原地处理
        std::vector<char> eq_data;

        if(eq.GetBandCount() > 0)
        {
            AudioDataInfo eq_info;

            if(openal::FromOpenALFormat(format, eq_info))
            {
                eq_info.sample_rate = freq;
                eq_info.data_size   = size;

                eq_data.resize(size);
                memcpy(eq_data.data(), data, size);

                if(ApplyEQToPCM(eq_data.data(), size, eq_info, eq))
                    data = eq_data.data();
            }
        }

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
            DecodedAudio *decoded=DecodeAudio(file_type,memory,size);

            if(!decoded)
            {
                alDeleteBuffers(1,&buffer_id);
                RETURN_FALSE;
            }

            const uint dec_sample_rate=decoded->freq;
            const double dec_duration=decoded->duration;

            // 应用 buffer 级 EQ（P2：解码后、上传前，EFX 缺失兜底）
            if(eq.GetBandCount() > 0)
            {
                AudioDataInfo eq_info;

                if(openal::FromOpenALFormat(decoded->format, eq_info))
                {
                    eq_info.sample_rate = decoded->freq;
                    eq_info.data_size   = decoded->size;

                    ApplyEQToPCM(decoded->data, decoded->size, eq_info, eq);
                }
            }

            if(!UploadDecoded(buffer_id,decoded))
            {
                alDeleteBuffers(1,&buffer_id);
                RETURN_FALSE;
            }

            sample_rate=dec_sample_rate;
            duration=dec_duration;
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
