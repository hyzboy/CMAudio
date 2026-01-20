#pragma once

#include<hgl/thread/Thread.h>
#include<hgl/thread/ThreadMutex.h>
#include<hgl/audio/OpenAL.h>
#include<hgl/audio/AudioSource.h>
#include<hgl/math/Vector.h>
#include<hgl/time/Time.h>

using namespace openal;

namespace hgl
{
    using namespace math;

    namespace io
    {
        class InputStream;
    }//namespace io

    struct AudioPlugInInterface;
    struct AudioMidiInterface;
    struct AudioMidiChannelInterface;
    struct MidiChannelInfo;

    enum class MIDIPlayState        //MIDI播放器状态
    {
        None=0,
        Play,
        Pause,
        Exit
    };

    /**
    * 专业MIDI播放器类，提供对MIDI文件的完整控制
    * 
    * 特性：
    * - 实时通道控制（音量、声像、静音、独奏）
    * - 动态乐器更换
    * - 音色库/音色选择
    * - 多通道分离解码
    * - 通道信息查询
    * - 独立的播放线程
    * 
    * 使用场景：
    * - 音乐制作和混音
    * - 卡拉OK系统
    * - 音乐学习软件
    * - 游戏背景音乐（动态混音）
    * - MIDI文件多轨导出
    */
    class MIDIPlayer:public Thread                                                                  ///专业MIDI播放器类
    {
        OBJECT_LOGGER

        ThreadMutex lock;

    protected:

        ALbyte *audio_data;
        int audio_data_size;

        void *audio_ptr;                                                                            ///<音频数据指针

        char *audio_buffer;
        int audio_buffer_size;
        uint audio_buffer_count;                                                                    ///<播放数据计数

        AudioPlugInInterface *decode;
        AudioMidiInterface *midi_config;                                                            ///<MIDI配置接口
        AudioMidiChannelInterface *midi_channels;                                                   ///<MIDI通道接口

        ALenum format;                                                                              ///<音频数据格式
        ALsizei rate;                                                                               ///<音频数据采样率

        struct
        {
            bool open;
            uint64 time;
            float gap;

            struct
            {
                float gain;
                double time;
            }start,end;
        }auto_gain;                                                                                 ///<自动增益

        bool ReadData(ALuint);
        bool UpdateBuffer();
        void ClearBuffer();

        bool Playback();

        bool DeletedAfterExit()const override{return false;}    
        bool Execute() override;

        void InitPrivate();
        bool LoadMIDI(const os_char *filename);
        bool LoadMIDI(io::InputStream *stream,int size);

    protected:

        volatile bool loop;
        volatile MIDIPlayState ps;

        AudioSource audiosource;
        ALuint source;
        ALuint buffer[3];
        PreciseTime total_time;
        PreciseTime wait_time;

        double gain;

        PreciseTime start_time;

        PreciseTime fade_in_time;
        PreciseTime fade_out_time;

    public: //播放器状态属性

                        uint            GetIndex()const{return audiosource.GetIndex();}             ///<获取音源索引

                        double          GetTotalTime()const{return total_time;}                     ///<获取音频总时长

                        MIDIPlayState   GetPlayState()const{return ps;}                             ///<获取播放器状态

                        int             GetSourceState()const{return audiosource.GetState();}       ///<获取音源状态

                        bool            IsLoop();                                                   ///<是否循环播放
                        void            SetLoop(bool);                                              ///<设置循环播放

                        float           GetGain()const{return audiosource.GetGain();}               ///<获取音量增益
                        float           GetMinGain()const{return audiosource.GetMinGain();}         ///<获取音量最小增益
                        float           GetMaxGain()const{return audiosource.GetMaxGain();}         ///<获取音量最大增益

                        void            SetGain(float val){audiosource.SetGain(val);}               ///<设置音量增益

                        float           GetPitch()const{return audiosource.GetPitch();}             ///<获取播放频率
                        void            SetPitch(float val){audiosource.SetPitch(val);}             ///<设置播放频率

    public: //空间音频属性

        const Vector3f &        GetPosition()const{return audiosource.GetPosition();}
        void                    SetPosition(const Vector3f &pos){audiosource.SetPosition(pos);}

        const Vector3f &        GetVelocity()const{return audiosource.GetVelocity();}
        void                    SetVelocity(const Vector3f &vel){audiosource.SetVelocity(vel);}

        const Vector3f &        GetDirection()const{return audiosource.GetDirection();}
        void                    SetDirection(const Vector3f &dir){audiosource.SetDirection(dir);}

    public: //MIDI配置接口

        /**
         * 设置SoundFont文件路径 (FluidSynth/TinySoundFont)
         * @param path SoundFont文件路径 (.sf2)
         * @return 是否设置成功
         */
        bool SetSoundFont(const char *path);

        /**
         * 设置音色库ID (OPNMIDI/ADLMIDI)
         * @param bank_id 音色库ID
         * @return 是否设置成功
         */
        bool SetBank(int bank_id);

        /**
         * 设置音色库文件 (OPNMIDI/ADLMIDI)
         * @param path 音色库文件路径 (.wopn/.wopl)
         * @return 是否设置成功
         */
        bool SetBankFile(const char *path);

        /**
         * 设置采样率
         * @param rate 采样率 (如: 44100, 48000)
         * @return 是否设置成功
         */
        bool SetSampleRate(int rate);

        /**
         * 获取当前SoundFont路径
         * @return SoundFont路径，如果未设置则返回nullptr
         */
        const char* GetSoundFont()const;

        /**
         * 获取当前音色库ID
         * @return 音色库ID
         */
        int GetBank()const;

        /**
         * 获取当前音色库文件路径
         * @return 音色库文件路径，如果未设置则返回nullptr
         */
        const char* GetBankFile()const;

        /**
         * 获取当前采样率
         * @return 采样率
         */
        int GetSampleRate()const;

    public: //MIDI通道控制接口

        /**
         * 获取MIDI通道数量
         * @return 通道数量（通常为16）
         */
        int GetChannelCount()const;

        /**
         * 获取指定通道的详细信息
         * @param channel 通道编号 (0-15)
         * @param info 输出的通道信息结构
         * @return 是否获取成功
         */
        bool GetChannelInfo(int channel, MidiChannelInfo *info)const;

        /**
         * 设置通道乐器
         * @param channel 通道编号 (0-15)
         * @param program 乐器编号 (0-127, 0=钢琴)
         * @return 是否设置成功
         */
        bool SetChannelProgram(int channel, int program);

        /**
         * 设置通道音量
         * @param channel 通道编号 (0-15)
         * @param volume 音量 (0-127)
         * @return 是否设置成功
         */
        bool SetChannelVolume(int channel, int volume);

        /**
         * 设置通道声像
         * @param channel 通道编号 (0-15)
         * @param pan 声像 (0=左, 64=中, 127=右)
         * @return 是否设置成功
         */
        bool SetChannelPan(int channel, int pan);

        /**
         * 设置通道静音状态
         * @param channel 通道编号 (0-15)
         * @param mute true=静音, false=取消静音
         * @return 是否设置成功
         */
        bool SetChannelMute(int channel, bool mute);

        /**
         * 设置通道独奏状态
         * @param channel 通道编号 (0-15)
         * @param solo true=独奏（其他通道静音）, false=取消独奏
         * @return 是否设置成功
         */
        bool SetChannelSolo(int channel, bool solo);

        /**
         * 重置指定通道到默认状态
         * @param channel 通道编号 (0-15)
         * @return 是否重置成功
         */
        bool ResetChannel(int channel);

        /**
         * 重置所有通道到默认状态
         * @return 是否重置成功
         */
        bool ResetAllChannels();

        /**
         * 检查通道是否处于活跃状态（正在发声）
         * @param channel 通道编号 (0-15)
         * @return true=活跃, false=静默
         */
        bool GetChannelActivity(int channel)const;

    public: //高级功能：多通道解码

        /**
         * 将指定通道单独解码到PCM缓冲区
         * 注意：这是一个耗时操作，适合离线处理
         * @param channel 通道编号 (0-15)
         * @param buffer 输出PCM缓冲区指针的指针
         * @param samples 要解码的采样数
         * @return 实际解码的采样数，失败返回0
         */
        int DecodeChannel(int channel, int16_t **buffer, int samples);

        /**
         * 将所有通道分别解码到多个PCM缓冲区
         * 注意：这是一个耗时操作，适合离线处理
         * @param buffers 输出PCM缓冲区数组（每个通道一个缓冲区）
         * @param samples 要解码的采样数
         * @return 实际解码的采样数，失败返回0
         */
        int DecodeAllChannels(int16_t **buffers, int samples);

    public: //播放控制方法

        MIDIPlayer();
        MIDIPlayer(const os_char *filename);
        virtual ~MIDIPlayer();

        virtual bool Load(const os_char *filename);                                                 ///<加载MIDI文件
        virtual bool Load(io::InputStream *stream,int size);                                        ///<从流中加载MIDI文件

        virtual void Play(bool loop=true);                                                          ///<播放MIDI
        virtual void Stop();                                                                        ///<停止播放
        virtual void Pause();                                                                       ///<暂停播放
        virtual void Resume();                                                                      ///<继续播放
        virtual void Clear();                                                                       ///<清除MIDI数据

        virtual PreciseTime GetPlayTime();                                                          ///<取得已播放时间(单位秒)
        virtual void SetFadeTime(PreciseTime fade_in,PreciseTime fade_out);                         ///<设置淡入淡出时间

        virtual void AutoGain(float target_gain,PreciseTime duration,const PreciseTime cur_time);   ///<自动音量过渡
    };//class MIDIPlayer
}//namespace hgl
