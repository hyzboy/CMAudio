#ifndef HGL_AUDIO_DECODE_INCLUDE
#define HGL_AUDIO_DECODE_INCLUDE

#include<hgl/audio/OpenAL.h>
#include<hgl/audio/AudioFileType.h>
#include<hgl/type/String.h>

namespace hgl::audio
{
    using openal::ALboolean;
    using openal::ALbyte;
    using openal::ALenum;
    using openal::ALsizei;
    using openal::ALvoid;

    struct AudioPlugInInterface
    {
        void    (AL_APIENTRY *Load      )(ALbyte *,ALsizei,ALenum *,ALvoid **,ALsizei *,ALsizei *,ALboolean *);
        void    (AL_APIENTRY *Clear     )(ALenum,ALvoid *,ALsizei,ALsizei);

        void *  (AL_APIENTRY *Open      )(ALbyte *,ALsizei,ALenum *,ALsizei *,double *);
        void    (AL_APIENTRY *Close     )(void *);
        uint    (AL_APIENTRY *Read      )(void *,char *,uint);
        void    (AL_APIENTRY *Restart   )(void *);
    };//struct AudioPlugInInterface

    struct AudioFloatPlugInInterface
    {
        void    (AL_APIENTRY *Load      )(ALbyte *,ALsizei,ALenum *,float **,ALsizei *,ALsizei *,ALboolean *);
        void    (AL_APIENTRY *Clear     )(ALenum,ALvoid *,ALsizei,ALsizei);

        uint    (AL_APIENTRY *Read      )(void *,float *,uint);
    };//struct AudioFloatPlugInInterface

    struct AudioMidiConfigInterface
    {
        void    (AL_APIENTRY *SetSoundFont      )(const char *);      // Set soundfont/bank file path for MIDI synthesis
        void    (AL_APIENTRY *SetBankID         )(int);               // Set bank ID for chip emulators (OPNMIDI/ADLMIDI)
        void    (AL_APIENTRY *SetVolume         )(float);             // Set global volume (0.0-1.0)
        void    (AL_APIENTRY *SetSampleRate     )(int);               // Set sample rate (default: 44100)
        void    (AL_APIENTRY *SetPolyphony      )(int);               // Set max polyphony/voices (FluidSynth only)
        void    (AL_APIENTRY *SetChipCount      )(int);               // Set number of emulated chips (OPNMIDI/ADLMIDI)
        void    (AL_APIENTRY *EnableReverb      )(bool);              // Enable/disable reverb (FluidSynth only)
        void    (AL_APIENTRY *EnableChorus      )(bool);              // Enable/disable chorus (FluidSynth only)
        const char* (AL_APIENTRY *GetVersionString)();                // Get plugin version string
        const char* (AL_APIENTRY *GetDefaultBank)();                  // Get default bank/soundfont path
    };//struct AudioMidiConfigInterface

    struct MIDIChannelInfo
    {
        int channel;                    // Channel number (0-15)
        int program;                    // Current program/instrument (0-127)
        int bank;                       // Bank number (0-16383)
        float volume;                   // Channel volume (0.0-1.0)
        float pan;                      // Pan position (-1.0=left, 0.0=center, 1.0=right)
        bool muted;                     // Is channel muted
        bool solo;                      // Is channel soloed
        int note_count;                 // Number of active notes on this channel
        const char* instrument_name;    // Name of current instrument (if available)
    };//struct MIDIChannelInfo

    struct AudioMidiChannelInterface
    {
        // Channel information
        int     (AL_APIENTRY *GetChannelCount   )();                          // Get total number of MIDI channels (usually 16)
        bool    (AL_APIENTRY *GetChannelInfo    )(int channel, MIDIChannelInfo*);  // Get information about a specific channel
        
        // Channel control
        void    (AL_APIENTRY *SetChannelProgram )(int channel, int program);  // Change instrument on a channel (0-127)
        void    (AL_APIENTRY *SetChannelBank    )(int channel, int bank);     // Change bank for a channel
        void    (AL_APIENTRY *SetChannelVolume  )(int channel, float volume); // Set channel volume (0.0-1.0)
        void    (AL_APIENTRY *SetChannelPan     )(int channel, float pan);    // Set channel pan (-1.0 to 1.0)
        void    (AL_APIENTRY *MuteChannel       )(int channel, bool mute);    // Mute/unmute a channel
        void    (AL_APIENTRY *SoloChannel       )(int channel, bool solo);    // Solo/unsolo a channel
        
        // Multi-channel decoding
        void*   (AL_APIENTRY *OpenMultiChannel  )(ALbyte *data, ALsizei size, ALenum *format, ALsizei *freq, double *time);  // Open for multi-channel decode
        uint    (AL_APIENTRY *ReadChannel       )(void *stream, int channel, char *pcm_data, uint buf_size);  // Read specific channel
        uint    (AL_APIENTRY *ReadChannels      )(void *stream, int *channels, int channel_count, char **pcm_data, uint buf_size);  // Read multiple channels separately
        void    (AL_APIENTRY *CloseMultiChannel )(void *stream);              // Close multi-channel stream
    };//struct AudioMidiChannelInterface

    /**
    * 音频编码插件接口（ver=5，可选能力，同浮点解码\"不是谁都有\"）
    *
    * 与解码接口（ver=2/3 文件→PCM）对称：编码接口做 PCM↔压缩包 的流式编解码，
    * 供实时通话链（CaptureSource→预处理→编码→网络→解码→播放）使用。
    * 只有具备编码能力的插件才实现本接口（GetInterface(5) 返回 false 即无编码能力）。
    *
    * 约定：
    * - Encode 输入 float PCM（-1..1），返回压缩包字节数；负数为编码错误码
    * - Decode 输入压缩包，输出 float PCM，返回样本数；packet==nullptr 表示丢包，走 PLC 隐藏
    */
    struct AudioCodecPlugInInterface
    {
        void *  (AL_APIENTRY *OpenEncoder      )(uint sample_rate,uint channels,uint bitrate,int *error);   ///< 打开编码器（bitrate=0 用默认），返回编码器句柄
        int     (AL_APIENTRY *Encode           )(void *enc,const float *pcm,uint frame_samples,char *packet,uint packet_cap);  ///< 编码一帧 PCM→压缩包
        void    (AL_APIENTRY *CloseEncoder     )(void *enc);                                                  ///< 关闭编码器

        void *  (AL_APIENTRY *OpenDecoder      )(uint sample_rate,uint channels,int *error);                 ///< 打开解码器
        int     (AL_APIENTRY *Decode           )(void *dec,const char *packet,int packet_size,float *pcm,uint pcm_cap);  ///< 解码一包→PCM（packet=nullptr→PLC）
        void    (AL_APIENTRY *CloseDecoder     )(void *dec);                                                  ///< 关闭解码器
    };//struct AudioCodecPlugInInterface

    bool GetAudioInterface(const OSString &,AudioPlugInInterface *,AudioFloatPlugInInterface *);
    bool GetAudioCodecInterface(const OSString &,AudioCodecPlugInInterface *);
    bool GetAudioMidiInterface(const OSString &,AudioMidiConfigInterface *);
    bool GetAudioMidiChannelInterface(const OSString &,AudioMidiChannelInterface *);

    const OSString *GetAudioPluginNameByExtension(const char *ext_name);   ///<动态：按文件扩展名查找音频解码插件(基于插件 FileExtensions 能力)

    /**
    * 解码结果（后台线程产出，主线程上传）
    * 持有插件解码得到的 PCM 数据与插件接口（用于释放）
    */
    struct DecodedAudio
    {
        AudioPlugInInterface decode{};          ///< 整型解码插件接口（用于 Release）
        AudioFloatPlugInInterface decode_float{};///< 浮点解码插件接口（用于 Release）
        bool use_float=false;                   ///< 用哪个接口释放
        ALenum format=0;                        ///< OpenAL 格式
        void *data=nullptr;                     ///< 解码得到的 PCM 数据（插件分配）
        ALsizei size=0;                         ///< 数据字节数
        ALsizei freq=0;                         ///< 采样率
        double duration=0;                      ///< 可播放时长(秒)

        void Release();                         ///< 释放插件解码数据
    };//struct DecodedAudio

    /**
    * 纯解码：从内存中的音频文件数据解码为 PCM（不碰 OpenAL，线程安全）
    * @param file_type 音频文件类型
    * @param memory 文件数据内存
    * @param memory_size 文件数据字节数
    * @return 解码结果（new 分配），失败返回 nullptr；调用方负责 Release+delete，或交给 UploadDecoded
    */
    DecodedAudio *DecodeAudio(AudioFileType file_type, void *memory, int memory_size);

    /**
    * 上传解码结果到 OpenAL buffer（需 current OpenAL context，应在主线程调用）
    * 上传成功后接管并释放 decoded（Release + delete）
    * @param buffer_id OpenAL buffer id
    * @param decoded 解码结果（非空）
    * @return 是否上传成功
    */
    bool UploadDecoded(uint buffer_id, DecodedAudio *decoded);
}//namespace hgl::audio
#endif//HGL_AUDIO_DECODE_INCLUDE
