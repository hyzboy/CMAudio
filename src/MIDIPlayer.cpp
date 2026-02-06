#include<hgl/audio/MIDIPlayer.h>
#include<hgl/log/Log.h>
#include<hgl/plugin/PlugIn.h>
#include<hgl/io/MemoryInputStream.h>
#include<hgl/io/FileInputStream.h>
#include"AudioDecode.h"

namespace hgl
{
    void MIDIPlayer::InitPrivate()
    {
        if(!alGenSources)
        {
            LogError(OS_TEXT("OpenAL/EE 还未初始化!"));
            return;
        }

        auto_gain.open=false;

        audio_ptr=nullptr;

        audio_data=nullptr;
        audio_data_size=0;

        audio_buffer=nullptr;
        audio_buffer_size=0;

        decode=nullptr;
        midi_config=nullptr;
        midi_channels=nullptr;

        ps=MIDIPlayState::None;

        total_time=0;

        if(!audiosource.Create())return;

        audiosource.SetLoop(false);

        source=audiosource.GetIndex();

        alGenBuffers(3,buffer);
    }

    MIDIPlayer::MIDIPlayer()
    {
        InitPrivate();
    }

    MIDIPlayer::MIDIPlayer(const os_char *filename)
    {
        InitPrivate();

        if(filename)
            Load(filename);
    }

    MIDIPlayer::~MIDIPlayer()
    {
        SAFE_CLEAR(decode);

        if(!audio_data)return;

        Clear();

        alDeleteBuffers(3,buffer);

        SAFE_CLEAR_ARRAY(audio_buffer);
    }

    bool MIDIPlayer::LoadMIDI(const os_char *filename)
    {
        // 强制使用MIDI解码器
        const os_char *plugin_name=OS_TEXT("MIDI");

        decode=new AudioPlugInInterface;

        if (!GetAudioInterface(plugin_name, decode, nullptr))
        {
            delete decode;
            decode=nullptr;

            LogError(OS_TEXT("无法加载MIDI解码插件：MIDI"));
            return(false);
        }

        // 获取MIDI配置接口
        if(GetAudioMidiInterface(plugin_name, &midi_config_storage))
        {
            midi_config = &midi_config_storage;
        }
        else
        {
            midi_config = nullptr;
            LogWarning(OS_TEXT("MIDI配置接口不可用"));
        }

        // 获取MIDI通道接口
        if(GetAudioMidiChannelInterface(plugin_name, &midi_channels_storage))
        {
            midi_channels = &midi_channels_storage;
        }
        else
        {
            midi_channels = nullptr;
            LogWarning(OS_TEXT("MIDI通道控制接口不可用"));
        }

        {
            audio_ptr=decode->Open(audio_data,audio_data_size,&format,&rate,&total_time);

            if(!audio_ptr)
            {
                LogError(OS_TEXT("MIDI文件打开失败：") + OSString(filename));
                return(false);
            }

            audio_buffer_size=(AudioTime(format,rate)+9)/10;        // 1/10 秒

            if(audio_buffer)
                delete[] audio_buffer;

            audio_buffer=new char[audio_buffer_size];

            wait_time=0.1;

            if(wait_time>total_time/3.0f)
                wait_time=total_time/10.0f;

            return(true);
        }
    }

    bool MIDIPlayer::LoadMIDI(io::InputStream *stream,int size)
    {
        if(!alGenBuffers)return(false);
        if(!stream)return(false);
        if(size<=0)return(false);

        Clear();

        audio_data=new ALbyte[size];

        stream->Read(audio_data,size);

        audio_data_size=size;

        return LoadMIDI(OS_TEXT(""));
    }

    /**
    * 从文件中加载MIDI数据
    * @param filename MIDI文件名称
    * @return 是否加载成功
    */
    bool MIDIPlayer::Load(const os_char *filename)
    {
        if(!alGenBuffers)return(false);
        if(!filename||!(*filename))return(false);

        OpenFileInputStream fis(filename);

        if(!fis)
        {
            LogError(OS_TEXT("无法打开MIDI文件：") + OSString(filename));
            return(false);
        }

        return Load(fis,fis->Available());
    }

    /**
    * 从流中加载MIDI数据
    * @param stream 要加载数据的流
    * @param size MIDI数据长度
    * @return 是否加载成功
    */
    bool MIDIPlayer::Load(io::InputStream *stream,int size)
    {
        return LoadMIDI(stream,size);
    }

    void MIDIPlayer::Clear()
    {
        Stop();

        if(decode&&audio_ptr)
            decode->Close(audio_ptr);

        SAFE_CLEAR_ARRAY(audio_data);
        SAFE_CLEAR_ARRAY(audio_buffer);

        audio_ptr=nullptr;
        midi_config=nullptr;
        midi_channels=nullptr;

        total_time=0;
    }

    bool MIDIPlayer::IsLoop()
    {
        lock.Lock();
        bool rv=loop;
        lock.Unlock();

        return(rv);
    }

    void MIDIPlayer::SetLoop(bool val)
    {
        lock.Lock();
        loop=val;
        lock.Unlock();
    }

    bool MIDIPlayer::ReadData(ALuint n)
    {
        if(!decode)return(false);

        uint size;

        size=decode->Read(audio_ptr,audio_buffer,audio_buffer_size);

        if(size)
        {
            alBufferData(n,format,audio_buffer,size,rate);

            if(alLastError())return(false);

            return(true);
        }

        return(false);
    }

    bool MIDIPlayer::Playback()
    {
        if(!audio_data)return(false);
        if(!decode)return(false);

        alSourceStop(source);
        ClearBuffer();
        decode->Restart(audio_ptr);

        int count=0;

        audio_buffer_count=0;

        if(ReadData(buffer[0]))
        {
            count++;

            if(ReadData(buffer[1]))
                count++;

            if(ReadData(buffer[2]))
                count++;

            alSourceQueueBuffers(source,count,buffer);
            alSourcePlay(source);
            start_time=GetTimeSec();

            ps=MIDIPlayState::Play;
            return(true);
        }
        else
        {
            ps=MIDIPlayState::Exit;

            return(false);
        }
    }

    /**
    * 开始播放MIDI
    * @param _loop 是否循环播放
    */
    void MIDIPlayer::Play(bool _loop)
    {
        if(!audio_data)return;

        lock.Lock();

        loop=_loop;

        if(ps==MIDIPlayState::None||ps==MIDIPlayState::Pause)
            Start();

        Playback();

        lock.Unlock();
    }

    /**
    * 停止播放
    */
    void MIDIPlayer::Stop()
    {
        if(!audio_data)return;

        bool thread_is_live=true;

        lock.Lock();

        if(Thread::IsLive())
            ps=MIDIPlayState::Exit;
        else
            thread_is_live=false;

        lock.Unlock();

        if(thread_is_live)
            Thread::WaitExit();

        ps=MIDIPlayState::None;
    }

    /**
    * 暂停播放
    */
    void MIDIPlayer::Pause()
    {
        if(!audio_data)return;

        lock.Lock();

        if(ps==MIDIPlayState::Play)
            ps=MIDIPlayState::Pause;

        lock.Unlock();
    }

    /**
    * 继续播放
    */
    void MIDIPlayer::Resume()
    {
        if(!audio_data)return;

        lock.Lock();

        if(ps==MIDIPlayState::Pause)
        {
            ps=MIDIPlayState::Play;

            Thread::Start();
        }

        lock.Unlock();
    }

    void MIDIPlayer::ClearBuffer()
    {
        ALint queued = 0;

        while(alGetSourcei(source,AL_BUFFERS_QUEUED,&queued),queued > 0)
        {
            ALuint n;
            alSourceUnqueueBuffers(source,1,&n);
            --queued;
        }
    }

    bool MIDIPlayer::UpdateBuffer()
    {
        int processed=0;
        bool active=true;

        alGetSourcei(source,AL_BUFFERS_PROCESSED,&processed);

        while(processed--)
        {
            ALuint n;

            alSourceUnqueueBuffers(source,1,&n);

            if(!ReadData(n))
            {
                active=false;
            }
            else
            {
                alSourceQueueBuffers(source,1,&n);
            }

            ++audio_buffer_count;
        }

        return active;
    }

    bool MIDIPlayer::Execute()
    {
        while(true)
        {
            lock.Lock();

            if(ps==MIDIPlayState::Exit)
            {
                alSourceStop(source);
                ClearBuffer();

                lock.Unlock();
                return(false);
            }

            if(ps==MIDIPlayState::Pause)
            {
                alSourcePause(source);

                lock.Unlock();

                SleepSecond(wait_time);
                continue;
            }

            if(ps==MIDIPlayState::Play)
            {
                if(!UpdateBuffer())
                {
                    if(loop)
                    {
                        Playback();
                    }
                    else
                    {
                        ps=MIDIPlayState::Exit;
                        lock.Unlock();
                        return(false);
                    }
                }

                // 处理自动增益
                if(auto_gain.open)
                {
                    uint64 cur_time=GetTimeSec();

                    if(cur_time>=auto_gain.time)
                    {
                        audiosource.SetGain(auto_gain.end.gain);
                        auto_gain.open=false;
                    }
                    else
                    {
                        double gap=(cur_time-auto_gain.start.time)/auto_gain.gap;

                        audiosource.SetGain(auto_gain.start.gain+(auto_gain.end.gain-auto_gain.start.gain)*gap);
                    }
                }

                // 处理淡入淡出
                if(fade_in_time>0||fade_out_time>0)
                {
                    double cur_pos=GetPlayTime();

                    if(fade_in_time>0&&cur_pos<fade_in_time)
                    {
                        float fade_gain=cur_pos/fade_in_time;
                        audiosource.SetGain(fade_gain*gain);
                    }
                    else if(fade_out_time>0&&cur_pos>(total_time-fade_out_time))
                    {
                        float fade_gain=(total_time-cur_pos)/fade_out_time;
                        audiosource.SetGain(fade_gain*gain);
                    }
                }
            }

            lock.Unlock();

            SleepSecond(wait_time);
        }

        return(true);
    }

    PreciseTime MIDIPlayer::GetPlayTime()
    {
        if(ps!=MIDIPlayState::Play)
            return 0;

        return GetTimeSec()-start_time+(audio_buffer_count*wait_time);
    }

    void MIDIPlayer::SetFadeTime(PreciseTime fade_in,PreciseTime fade_out)
    {
        lock.Lock();

        fade_in_time=fade_in;
        fade_out_time=fade_out;

        lock.Unlock();
    }

    void MIDIPlayer::AutoGain(float target_gain,PreciseTime duration,const PreciseTime cur_time)
    {
        lock.Lock();

        auto_gain.open=true;
        auto_gain.start.gain=audiosource.GetGain();
        auto_gain.start.time=cur_time;
        auto_gain.end.gain=target_gain;
        auto_gain.end.time=cur_time+duration;
        auto_gain.gap=duration;
        auto_gain.time=cur_time+duration;

        lock.Unlock();
    }

    // ====================================================================
    // MIDI配置接口实现
    // ====================================================================

    bool MIDIPlayer::SetSoundFont(const char *path)
    {
        if(!midi_config)return false;
        if(!midi_config->SetSoundFont)return false;

        midi_config->SetSoundFont(path);
        current_soundfont = path ? path : "";
        return true;
    }

    bool MIDIPlayer::SetBank(int bank_id)
    {
        if(!midi_config)return false;
        if(!midi_config->SetBankID)return false;

        midi_config->SetBankID(bank_id);
        current_bank_id = bank_id;
        return true;
    }

    bool MIDIPlayer::SetBankFile(const char *path)
    {
        if(!midi_config)return false;
        if(!midi_config->SetSoundFont)return false;

        midi_config->SetSoundFont(path);
        current_soundfont = path ? path : "";
        current_bank_file = path ? path : "";
        return true;
    }

    bool MIDIPlayer::SetSampleRate(int rate)
    {
        if(!midi_config)return false;
        if(!midi_config->SetSampleRate)return false;

        midi_config->SetSampleRate(rate);
        current_sample_rate = rate;
        return true;
    }

    const char* MIDIPlayer::GetSoundFont()const
    {
        if(!midi_config)return nullptr;
        if(!current_soundfont.empty())
            return current_soundfont.c_str();
        if(midi_config->GetDefaultBank)
            return midi_config->GetDefaultBank();
        return nullptr;
    }

    int MIDIPlayer::GetBank()const
    {
        return current_bank_id;
    }

    const char* MIDIPlayer::GetBankFile()const
    {
        if(!current_bank_file.empty())
            return current_bank_file.c_str();
        if(midi_config && midi_config->GetDefaultBank)
            return midi_config->GetDefaultBank();
        return nullptr;
    }

    int MIDIPlayer::GetSampleRate()const
    {
        return current_sample_rate;
    }

    // ====================================================================
    // MIDI通道控制接口实现
    // ====================================================================

    int MIDIPlayer::GetChannelCount()const
    {
        if(!midi_channels)return 0;
        if(!midi_channels->GetChannelCount)return 0;

        return midi_channels->GetChannelCount();
    }

    bool MIDIPlayer::GetChannelInfo(int channel, MidiChannelInfo *info)const
    {
        if(!midi_channels)return false;
        if(!midi_channels->GetChannelInfo)return false;

        return midi_channels->GetChannelInfo(channel,info);
    }

    bool MIDIPlayer::SetChannelProgram(int channel, int program)
    {
        if(!midi_channels)return false;
        if(!midi_channels->SetChannelProgram)return false;

        midi_channels->SetChannelProgram(channel,program);
        return true;
    }

    bool MIDIPlayer::SetChannelVolume(int channel, int volume)
    {
        if(!midi_channels)return false;
        if(!midi_channels->SetChannelVolume)return false;

        midi_channels->SetChannelVolume(channel,(float)volume);
        return true;
    }

    bool MIDIPlayer::SetChannelPan(int channel, int pan)
    {
        if(!midi_channels)return false;
        if(!midi_channels->SetChannelPan)return false;

        midi_channels->SetChannelPan(channel,(float)pan);
        return true;
    }

    bool MIDIPlayer::SetChannelMute(int channel, bool mute)
    {
        if(!midi_channels)return false;
        if(!midi_channels->MuteChannel)return false;

        midi_channels->MuteChannel(channel,mute);
        return true;
    }

    bool MIDIPlayer::SetChannelSolo(int channel, bool solo)
    {
        if(!midi_channels)return false;
        if(!midi_channels->SoloChannel)return false;

        midi_channels->SoloChannel(channel,solo);
        return true;
    }

    bool MIDIPlayer::ResetChannel(int channel)
    {
        (void)channel;
        return false;
    }

    bool MIDIPlayer::ResetAllChannels()
    {
        return false;
    }

    bool MIDIPlayer::GetChannelActivity(int channel)const
    {
        MidiChannelInfo info={0};
        if(!midi_channels)
            return false;
        if(!midi_channels->GetChannelInfo)
            return false;
        if(!midi_channels->GetChannelInfo(channel, &info))
            return false;
        return info.note_count > 0;
    }

    int MIDIPlayer::DecodeChannel(int channel, int16_t **buffer, int samples)
    {
        if(!midi_channels || !midi_channels->OpenMultiChannel || !midi_channels->ReadChannel || !midi_channels->CloseMultiChannel)
            return 0;
        if(!audio_data || audio_data_size <= 0 || !buffer || samples <= 0)
            return 0;

        if(!*buffer)
            *buffer = new int16_t[samples];

        double total_time=0;
        void* stream = midi_channels->OpenMultiChannel(audio_data, audio_data_size, &format, &rate, &total_time);
        if(!stream)
            return 0;

        uint bytes_read = midi_channels->ReadChannel(stream, channel, reinterpret_cast<char*>(*buffer), (uint)(samples * sizeof(int16_t)));
        midi_channels->CloseMultiChannel(stream);
        return (int)(bytes_read / sizeof(int16_t));
    }

    int MIDIPlayer::DecodeAllChannels(int16_t **buffers, int samples)
    {
        if(!midi_channels || !midi_channels->OpenMultiChannel || !midi_channels->ReadChannels || !midi_channels->CloseMultiChannel)
            return 0;
        if(!audio_data || audio_data_size <= 0 || !buffers || samples <= 0)
            return 0;

        double total_time=0;
        void* stream = midi_channels->OpenMultiChannel(audio_data, audio_data_size, &format, &rate, &total_time);
        if(!stream)
            return 0;

        int channel_count = 16;
        int channels[16];
        char* pcm_buffers[16];

        for(int i=0;i<channel_count;i++)
        {
            channels[i]=i;
            if(!buffers[i])
                buffers[i]=new int16_t[samples];
            pcm_buffers[i]=reinterpret_cast<char*>(buffers[i]);
        }

        uint bytes_read = midi_channels->ReadChannels(stream, channels, channel_count, pcm_buffers, (uint)(samples * sizeof(int16_t)));
        midi_channels->CloseMultiChannel(stream);
        return (int)(bytes_read / sizeof(int16_t));
    }

}//namespace hgl
