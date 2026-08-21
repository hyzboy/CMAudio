#include<hgl/audio/AudioPlayer.h>
#include<hgl/audio/CaptureSource.h>
#include<hgl/log/Log.h>
#include<hgl/plugin/PlugIn.h>
#include<hgl/io/MemoryInputStream.h>
#include<hgl/io/FileInputStream.h>
#include"AudioDecode.h"

using namespace openal;

namespace hgl::audio
{
    const os_char *GetAudioDecodeName(const AudioFileType aft);

    void AudioPlayer::InitPrivate()
    {
        if(!alGenSources)
        {
            LogError(OS_TEXT("OpenAL/EE 还未初始化!"));
            return;
        }

        fade_in_time=0;
        fade_out_time=0;

        gain=1.0;

        audio_ptr=nullptr;

        audio_data=nullptr;
        audio_data_size=0;

        audio_buffer=nullptr;
        audio_buffer_size=0;

        decoder=nullptr;

        play_state=PlayState::None;

        total_time=0;

        capture=nullptr;
        realtime_source=false;

        loop=false;             // atom<bool> 默认构造不初始化，必须显式置位（否则 Execute 的 if(loop) 读垃圾值循环播放）

        if(!audiosource.Create())return;

        audiosource.SetLoop(false);

        source_id=audiosource.GetIndex();

        alGenBuffers(3,al_buffers);
    }

    AudioPlayer::AudioPlayer()
    {
        InitPrivate();
    }

    AudioPlayer::AudioPlayer(const os_char *filename,AudioFileType aft)
    {
        InitPrivate();

        if(filename)
            Load(filename,aft);
    }

    AudioPlayer::AudioPlayer(InputStream *stream,int size,AudioFileType aft)
    {
        InitPrivate();

        Load(stream,size,aft);
    }

//     AudioPlayer::AudioPlayer(HAC *hac,const os_char *filename,AudioFileType aft)
//     {
//         InitPrivate();
//
//         Load(hac,filename,aft);
//     }

    AudioPlayer::~AudioPlayer()
    {
        SAFE_CLEAR(decoder);

        if(!audio_data&&!realtime_source)return;

        Clear();

        alDeleteBuffers(3,al_buffers);

        SAFE_CLEAR_ARRAY(audio_buffer);
    }

    bool AudioPlayer::Load(AudioFileType aft)
    {
        const os_char *plugin_name=GetAudioDecodeName(aft);

        if(!plugin_name)return(false);

        decoder=new AudioPlugInInterface;

        if (!GetAudioInterface(plugin_name, decoder, nullptr))
        {
            delete decoder;
            decoder=nullptr;

            LogError(OS_TEXT("无法加载音频解码插件：")+OSString(plugin_name));
            return(false);
        }

        {
            double open_total_time=0;

            audio_ptr=decoder->Open(audio_data,audio_data_size,&al_format,&sample_rate,&open_total_time);

            total_time=open_total_time;

            audio_buffer_size=(AudioTime(al_format,sample_rate)+9)/10;        // 1/10 秒

            if(audio_buffer)
                delete[] audio_buffer;

            audio_buffer=new char[audio_buffer_size];

            wait_time=0.1;

            if(wait_time>total_time.load()/3.0f)
                wait_time=total_time.load()/10.0f;

            return(true);
        }
    }

    /**
    * 从流中加载音频数据,仅支持OGG
    * @param stream 要加载数据的流
    * @param size 音频数据长度
    * @param aft 音频文件类型
    * @return 是否加载成功
    */
    bool AudioPlayer::Load(InputStream *stream,int size,AudioFileType aft)
    {
        if(!alGenBuffers)return(false);
        if(!stream)return(false);
        if(size<=0)return(false);

        Clear();

        if(!RangeCheck(aft))
        {
            LogError(OS_TEXT("未支持的音频文件类型！AudioFileType: ")+OSString::numberOf((int)aft));

            return(false);
        }
        else
        {
            audio_data=new ALbyte[size];

            stream->Read(audio_data,size);

            audio_data_size=size;
            return Load(aft);
        }
    }

    /**
    * 从文件中加载一段音频数据
    * @param filename 音频文件名称
    * @param aft 音频文件类型
    * @return 是否加载成功
    */
    bool AudioPlayer::Load(const os_char *filename,AudioFileType aft)
    {
        if(!alGenBuffers)return(false);
        if(!filename||!(*filename))return(false);

        if(!RangeCheck(aft))
            aft=CheckAudioFileType(filename);

        if(!RangeCheck(aft))
        {
            LogError(OS_TEXT("未知的音频文件类型！AudioFile: ")+OSString(filename));
            return(false);
        }

        io::OpenFileInputStream file_stream(filename);

        return(Load(file_stream,file_stream->Available(),aft));
    }

    bool AudioPlayer::LoadCapture(uint sr,uint frame_ms,bool use_mock)
    {
        if(!alGenBuffers)
            return(false);

        Clear();

        capture=new CaptureSource;

        if(!capture->Open(sr,frame_ms,AL_FORMAT_MONO16,use_mock))
        {
            SAFE_CLEAR(capture);
            return(false);
        }

        al_format=capture->GetFormat();
        this->sample_rate=capture->GetSampleRate();

        total_time=0;                       // 实时流：时长未知
        realtime_source=true;

        audio_buffer_size=capture->GetFrameBytes();         // 一帧大小
        audio_buffer=new char[audio_buffer_size];

        wait_time=frame_ms/2000.0;          // 帧长一半（20ms 帧 → 10ms 轮询）

        return(true);
    }

    void AudioPlayer::Clear()
    {
        Stop();

        if(decoder&&audio_ptr)
            decoder->Close(audio_ptr);

        SAFE_CLEAR_ARRAY(audio_data);
        SAFE_CLEAR_ARRAY(audio_buffer);

        audio_ptr=nullptr;

        total_time=0;
        realtime_source=false;

        SAFE_CLEAR(capture);
    }

    bool AudioPlayer::IsLoop()
    {
        lock.Lock();
        bool rv=loop.load();
        lock.Unlock();

        return(rv);
    }

    void AudioPlayer::SetLoop(bool val)
    {
        lock.Lock();
        loop=val;
        lock.Unlock();
    }

    bool AudioPlayer::ReadData(ALuint n)
    {
        if(realtime_source)
        {
            if(!capture)
                return(false);

            const int bytes=capture->ReadFrame(audio_buffer,(int)capture->GetFrameSamples());

            if(bytes<=0)
                return(false);

            alBufferData(n,al_format,audio_buffer,bytes,sample_rate);

            if(alLastError())
                return(false);

            return(true);
        }

        if(!decoder)return(false);

        uint size;

        size=decoder->Read(audio_ptr,audio_buffer,audio_buffer_size);

        if(size)
        {
            alBufferData(n,al_format,audio_buffer,size,sample_rate);

            if(alLastError())return(false);

            return(true);
        }

        return(false);
    }

    bool AudioPlayer::Playback()
    {
        if(!audio_data&&!realtime_source)return(false);
        if(!decoder&&!realtime_source)return(false);

        alSourceStop(source_id);
        ClearBuffer();

        if(realtime_source)
            capture->Start();               // 重新开始采集
        else
            decoder->Restart(audio_ptr);

        int count=0;

        audio_buffer_count=0;

        if(ReadData(al_buffers[0]))
        {
            count++;

            if(ReadData(al_buffers[1]))                //以免有些音效太短，在这里直接失败
                count++;

            if(ReadData(al_buffers[2]))                //以免有些音效太短，在这里直接失败
                count++;

            alSourceQueueBuffers(source_id,count,al_buffers);
            alSourcePlay(source_id);
            start_time=GetTimeSec();

            play_state=PlayState::Play;
            return(true);
        }
        else
        {
            play_state=PlayState::Exit;

            return(false);
        }
    }

    /**
    * 开始播放
    * @param _loop 是否循环播放
    */
    void AudioPlayer::Play(bool _loop)
    {
        if(!audio_data&&!realtime_source)return;

        lock.Lock();

        loop=_loop;

        if(play_state.load()==PlayState::None||play_state.load()==PlayState::Pause)      //未启动线程
            Start();

        Playback();            //Execute执行有检测Lock，所以不必担心该操作会引起线程冲突

        lock.Unlock();
    }

    /**
    * 停止播放
    */
    void AudioPlayer::Stop()
    {
        if(!audio_data&&!realtime_source)return;

        bool thread_is_live=true;

        lock.Lock();

        if(Thread::IsLive())
            play_state=PlayState::Exit;
        else
            thread_is_live=false;

        lock.Unlock();

        if(thread_is_live)
            Thread::WaitExit();

        play_state=PlayState::None;
    }

    /**
    * 暂停播放
    */
    void AudioPlayer::Pause()
    {
        if(!audio_data&&!realtime_source)return;

        lock.Lock();

        if(play_state.load()==PlayState::Play)
            play_state=PlayState::Pause;

        lock.Unlock();
    }

    /**
    * 继续播放
    */
    void AudioPlayer::Resume()
    {
        if(!audio_data&&!realtime_source)return;

        lock.Lock();

        if(play_state.load()==PlayState::Pause)
        {
            play_state=PlayState::Play;

            Thread::Start();
        }

        lock.Unlock();
    }

    bool AudioPlayer::UpdateBuffer()
    {
        int processed=0;
        bool active=true;

        alGetSourcei(source_id,AL_BUFFERS_PROCESSED,&processed);        //取得处理结束的缓冲区数量

        if(processed<=0)return(true);

        const PreciseTime cur_time=GetTimeSec();

        if(!realtime_source&&(fade_in_time>0||fade_out_time>0))
        {
            const float factor=FadeFactor(cur_time-start_time,fade_in_time,fade_out_time,total_time.load());

            audiosource.SetGain(float(factor*gain));
        }

        if(gain_ramp.active)
        {
            float g;

            if(!gain_ramp.Evaluate(cur_time,g))
            {
                SetGain(gain_ramp.end_gain);

                if(gain_ramp.end_gain<=0)
                    play_state=PlayState::Exit;
            }
            else
            {
                SetGain(g);
            }
        }

        while(processed--)
        {
            ALuint buffer;

            audio_buffer_count+=audio_buffer_size;

            alSourceUnqueueBuffers(source_id,1,&buffer);       //解除一个已处理完成的缓冲区
            alLastError();

            active=ReadData(buffer);                        //解码数据到这个缓冲区

            if(active)
            {
                alSourceQueueBuffers(source_id,1,&buffer);     //重新将这个缓冲区加入队列
                alLastError();
            }
            else
                return(false);
        }

        return(true);
    }

    void AudioPlayer::ClearBuffer()
    {
        int queued;
        ALuint buffer;

        alGetSourcei(source_id, AL_BUFFERS_QUEUED, &queued);

        while(queued--)
            alSourceUnqueueBuffers(source_id, 1, &buffer);
    }

    bool AudioPlayer::ProcStartThread()
    {
        // OpenAL current context 是 per-thread：播放线程必须绑定，
        // 否则 alGetSourcei(AL_BUFFERS_PROCESSED) 等调用全部失败 → processed 恒 0 → 播放永不推进
        openal::alcSetDefaultContext();

        return hgl::Thread::ProcStartThread();
    }

    bool AudioPlayer::Execute()
    {
        if(!audio_data&&!realtime_source)return(false);

        while(true)
        {
            lock.Lock();

            if(play_state.load()==PlayState::Play)    //被要求播放
            {
                if(!UpdateBuffer())
                {
                    if(loop)        //被要求循环播放
                    {
                        if(GetSourceState()!=AL_STOPPED)               //等它放完
                            Playback();
                    }
                    else
                    {
                        if(realtime_source)
                        {
                            // 实时源暂无数据：继续等待（不退出线程）
                            lock.Unlock();

                            SleepSecond(wait_time);

                            continue;
                        }

                        //退出
                        lock.Unlock();

                        play_state=PlayState::None;
                        return(false);
                    }
                }
                else
                {
                    if(GetSourceState()!=AL_PLAYING)
                        alSourcePlay(source_id);
                }
            }
            else
            if(play_state.load()==PlayState::Pause)        //被要求暂停
            {
                alSourcePause(source_id);

                lock.Unlock();
                return(false);
            }
            else
            if(play_state.load()==PlayState::Exit)      //被要求退出
            {
                alSourceStop(source_id);
                alSourcei(source_id,AL_BUFFER,0);
                ClearBuffer();

                lock.Unlock();
                return(false);
            }

            lock.Unlock();

            SleepSecond(wait_time);      //以让线程空出CPU时间片
        }
    }

    PreciseTime AudioPlayer::GetPlayTime()
    {
        if(!audio_data&&!realtime_source)return(0);

        uint base;
        int off;

        lock.Lock();

        base=audio_buffer_count;

        alGetSourcei(source_id,AL_BYTE_OFFSET,&off);

        lock.Unlock();

        return AudioDataTime(base+off,al_format,sample_rate);
    }

    /**
    * 自动调整增益
    * @param target_gain 目标增益
    * @param adjust_time 到达目标增益要经过的时间
    * @param cur_time 当前时间
    */
    void AudioPlayer::AutoGain(float target_gain,PreciseTime adjust_time,const PreciseTime cur_time)
    {
        if(!audio_data&&!realtime_source)return;

        lock.Lock();
            gain_ramp.Start(cur_time,GetGain(),target_gain,adjust_time);
        lock.Unlock();
    }

    void AudioPlayer::SetFadeTime(PreciseTime in,PreciseTime out)
    {
        fade_in_time=in;
        fade_out_time=out;
    }
}//namespace hgl::audio
