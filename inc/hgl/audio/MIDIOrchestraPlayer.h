#pragma once

#include<hgl/thread/Thread.h>
#include<hgl/thread/ThreadMutex.h>
#include<hgl/audio/OpenAL.h>
#include<hgl/audio/AudioSource.h>
#include<hgl/math/Vector.h>
#include<hgl/time/Time.h>
#include"AudioDecode.h"

using namespace openal;

namespace hgl
{
    using namespace math;

    namespace io
    {
        class InputStream;
    }//namespace io

    struct AudioPlugInInterface;
    struct AudioMidiConfigInterface;
    struct AudioMidiChannelInterface;
    struct MidiChannelInfo;

    class AudioManager;

    /**
     * 乐器的3D位置配置
     * Instrument 3D position configuration
     */
    struct OrchestraChannelPosition
    {
        Vector3f position;          ///< 3D位置 / 3D position
        float gain;                 ///< 音量增益 / Volume gain (0.0-1.0)
        bool enabled;               ///< 是否启用此通道 / Enable this channel
    };

    /**
     * 预设的乐队布局
     * Preset orchestra layouts
     */
    enum class OrchestraLayout
    {
        Standard,                   ///< 标准交响乐团布局 / Standard symphony layout
        Chamber,                    ///< 室内乐布局 / Chamber music layout
        Jazz,                       ///< 爵士乐布局 / Jazz ensemble layout
        Rock,                       ///< 摊滚乐队布局 / Rock band layout
        Custom                      ///< 自定义布局 / Custom layout
    };

    enum class MIDIOrchestraState   //Orchestra播放器状态 / Orchestra player state
    {
        None=0,
        Play,
        Pause,
        Exit
    };

    /**
    * 3D空间MIDI交响乐团播放器
    * 
    * 依赖要求 / Dependencies:
    * - 需要 FluidSynth 插件 (Audio.FluidSynth)
    * - FluidSynth 提供最佳音质和完整的多通道分离支持
    * 
    * Requires FluidSynth plugin (Audio.FluidSynth)
    * FluidSynth provides best audio quality and complete multi-channel separation
    * 
    * 特性 / Features:
    * - 每个MIDI通道独立的AudioSource和3D位置
    * - 模拟真实乐队的空间布局
    * - 支持多种预设布局（交响乐、室内乐、爵士、摇滚）
    * - 完美同步的多通道播放
    * - 实时位置调整
    * - 独立的距离衰减和空间音频效果
    * 
    * Each MIDI channel has independent AudioSource and 3D position
    * Simulates real orchestra spatial layout
    * Supports multiple preset layouts (Symphony, Chamber, Jazz, Rock)
    * Perfect synchronization across all channels
    * Real-time position adjustment
    * Independent distance attenuation and spatial audio effects
    * 
    * 使用场景 / Use Cases:
    * - VR音乐厅体验 / VR concert hall experience
    * - 游戏中的乐队表演 / Orchestra performance in games
    * - 音乐教育软件 / Music education software
    * - 沉浸式音频应用 / Immersive audio applications
    */
    class MIDIOrchestraPlayer:public Thread                                                     ///3D空间MIDI交响乐团播放器
    {
        OBJECT_LOGGER

        ThreadMutex lock;

    protected:

        static const int MAX_MIDI_CHANNELS = 16;                                               ///<MIDI通道数量

        ALbyte *audio_data;
        int audio_data_size;

        void *audio_ptr;                                                                        ///<音频数据指针

        char *channel_buffers[MAX_MIDI_CHANNELS];                                              ///<每个通道的音频缓冲
        int audio_buffer_size;
        uint audio_buffer_count;                                                               ///<播放数据计数

        AudioPlugInInterface *decode;
        AudioMidiConfigInterface *midi_config;                                                  ///<MIDI配置接口
        AudioMidiChannelInterface *midi_channels;                                              ///<MIDI通道接口

        AudioMidiConfigInterface midi_config_storage;                                           ///<MIDI配置接口存储
        AudioMidiChannelInterface midi_channels_storage;                                        ///<MIDI通道接口存储

        AudioManager *audio_manager;                                                            ///<音频管理器

        ALenum format;                                                                          ///<音频数据格式
        ALsizei rate;                                                                           ///<音频数据采样率

        AudioSource *sources[MAX_MIDI_CHANNELS];                                               ///<每个通道的AudioSource
        ALuint buffers[MAX_MIDI_CHANNELS][3];                                                  ///<每个通道3个缓冲区（三缓冲）
        OrchestraChannelPosition channel_positions[MAX_MIDI_CHANNELS];                         ///<每个通道的位置配置

        OrchestraLayout current_layout;                                                         ///<当前布局
        Vector3f orchestra_center;                                                              ///<乐队中心位置
        float orchestra_scale;                                                                  ///<乐队缩放比例

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
        }auto_gain;                                                                             ///<自动增益

        bool ReadChannelData(int channel, ALuint buffer_id);
        bool UpdateAllChannelBuffers();
        void ClearAllBuffers();

        bool PlaybackAllChannels();

        bool DeletedAfterExit()const override{return false;}    
        bool Execute() override;

        void InitPrivate();
        bool LoadMIDI(const os_char *filename);
        bool LoadMIDI(io::InputStream *stream,int size);

        void InitializeChannelSources();
        void ReleaseChannelSources();
        void ApplyLayoutPreset(OrchestraLayout layout);

    protected:

        bool play;
        bool pause_state;
        MIDIOrchestraState state;

    public: //属性 / Properties

        virtual const bool  IsNone      ()const{return state==MIDIOrchestraState::None;}       ///<是否未在播放 / Is not playing
        virtual const bool  IsPlaying   ()const{return state==MIDIOrchestraState::Play;}       ///<是否正在播放 / Is playing
        virtual const bool  IsPaused    ()const{return state==MIDIOrchestraState::Pause;}      ///<是否暂停 / Is paused

                const int   GetChannelCount();                                                  ///<获取MIDI通道数量 / Get MIDI channel count

                const MidiChannelInfo GetChannelInfo(int channel);                              ///<获取通道信息 / Get channel info

    public: //方法 / Methods

        MIDIOrchestraPlayer();
        virtual ~MIDIOrchestraPlayer();

        /**
         * 加载MIDI文件 / Load MIDI file
         * @param filename 文件名 / Filename
         * @return 是否成功 / Success
         */
        bool Load(const os_char *filename);

        /**
         * 从流加载MIDI / Load MIDI from stream
         * @param stream 输入流 / Input stream
         * @param size 数据大小 / Data size
         * @return 是否成功 / Success
         */
        bool Load(io::InputStream *stream,int size=-1);

        /**
         * 开始播放 / Start playback
         * @param loop 是否循环 / Loop playback
         */
        void Play(bool loop=false);

        /**
         * 暂停播放 / Pause playback
         */
        void Pause();

        /**
         * 继续播放 / Resume playback
         */
        void Resume();

        /**
         * 停止播放 / Stop playback
         */
        void Stop();

        /**
         * 关闭播放器 / Close player
         */
        void Close();

    public: //MIDI配置 / MIDI Configuration

        /**
         * 设置音色库 / Set SoundFont
         * @param path 音色库文件路径 / SoundFont file path
         */
        void SetSoundFont(const char *path);

        /**
         * 设置音色库ID / Set bank ID
         * @param bank_id 音色库ID / Bank ID
         */
        void SetBank(int bank_id);

        /**
         * 设置采样率 / Set sample rate
         * @param rate 采样率 / Sample rate
         */
        void SetSampleRate(int rate);

    public: //3D空间布局 / 3D Spatial Layout

        /**
         * 设置乐队布局 / Set orchestra layout
         * @param layout 布局类型 / Layout type
         */
        void SetLayout(OrchestraLayout layout);

        /**
         * 设置乐队中心位置 / Set orchestra center position
         * @param center 中心位置 / Center position
         */
        void SetOrchestraCenter(const Vector3f &center);

        /**
         * 设置乐队缩放 / Set orchestra scale
         * @param scale 缩放比例 / Scale factor
         */
        void SetOrchestraScale(float scale);

        /**
         * 设置通道的3D位置 / Set channel 3D position
         * @param channel 通道号(0-15) / Channel number (0-15)
         * @param position 3D位置 / 3D position
         */
        void SetChannelPosition(int channel, const Vector3f &position);

        /**
         * 获取通道的3D位置 / Get channel 3D position
         * @param channel 通道号(0-15) / Channel number (0-15)
         * @return 3D位置 / 3D position
         */
        Vector3f GetChannelPosition(int channel) const;

        /**
         * 设置通道音量 / Set channel volume
         * @param channel 通道号(0-15) / Channel number (0-15)
         * @param volume 音量(0.0-1.0) / Volume (0.0-1.0)
         */
        void SetChannelVolume(int channel, float volume);

        /**
         * 启用/禁用通道 / Enable/disable channel
         * @param channel 通道号(0-15) / Channel number (0-15)
         * @param enabled 是否启用 / Enable flag
         */
        void SetChannelEnabled(int channel, bool enabled);

        /**
         * 获取通道的AudioSource / Get channel AudioSource
         * @param channel 通道号(0-15) / Channel number (0-15)
         * @return AudioSource指针 / AudioSource pointer
         */
        AudioSource* GetChannelSource(int channel);

    public: //通道控制 / Channel Control

        /**
         * 设置通道乐器 / Set channel program (instrument)
         * @param channel 通道号(0-15) / Channel number (0-15)
         * @param program 乐器编号(0-127) / Program number (0-127)
         */
        void SetChannelProgram(int channel, int program);

        /**
         * 静音通道 / Mute channel
         * @param channel 通道号(0-15) / Channel number (0-15)
         * @param mute 是否静音 / Mute flag
         */
        void MuteChannel(int channel, bool mute);

        /**
         * 独奏通道 / Solo channel
         * @param channel 通道号(0-15) / Channel number (0-15)
         * @param solo 是否独奏 / Solo flag
         */
        void SoloChannel(int channel, bool solo);

    public: //自动增益 / Auto Gain

        /**
         * 设置自动增益 / Set auto gain
         * @param start_gain 起始增益 / Start gain
         * @param gap 间隔时间 / Gap time
         * @param end_gain 结束增益 / End gain
         */
        void AutoGain(float start_gain,float gap,float end_gain);

        /**
         * 自动淡入 / Auto fade in
         * @param gap 间隔时间 / Gap time
         */
        void FadeIn(float gap){AutoGain(0,gap,1);}

        /**
         * 自动淡出 / Auto fade out
         * @param gap 间隔时间 / Gap time
         */
        void FadeOut(float gap){AutoGain(1,gap,0);}
    };//class MIDIOrchestraPlayer

}//namespace hgl
