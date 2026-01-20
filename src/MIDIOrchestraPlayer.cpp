#include<hgl/audio/MIDIOrchestraPlayer.h>
#include<hgl/log/Log.h>
#include<hgl/plugin/PlugIn.h>
#include<hgl/io/MemoryInputStream.h>
#include<hgl/io/FileInputStream.h>
#include<hgl/audio/AudioManager.h>
#include"AudioDecode.h"

namespace hgl
{
    // 标准交响乐团布局（单位：米）
    // Standard symphony orchestra layout (in meters)
    static const OrchestraChannelPosition StandardOrchestraLayout[16] = {
        {{0, 0, -2}, 1.0f, true},      // 0: Piano (center front)
        {{-1.5f, 0, -1}, 1.0f, true},  // 1: Violin I (left front)
        {{1.5f, 0, -1}, 1.0f, true},   // 2: Violin II (right front)
        {{-2, 0, 0}, 1.0f, true},      // 3: Viola (left center)
        {{2, 0, 0}, 1.0f, true},       // 4: Cello (right center)
        {{0, 0, 1}, 1.0f, true},       // 5: Bass (center back)
        {{-2.5f, 0, -2}, 1.0f, true},  // 6: Flute (left front)
        {{2.5f, 0, -2}, 1.0f, true},   // 7: Oboe (right front)
        {{-3, 0, 0}, 1.0f, true},      // 8: Clarinet (left center)
        {{3, 0, 0}, 1.0f, true},       // 9: Drums (right center, typically channel 10)
        {{-2.5f, 0, 1}, 1.0f, true},   // 10: Trumpet (left back)
        {{2.5f, 0, 1}, 1.0f, true},    // 11: Trombone (right back)
        {{-3.5f, 0, 1.5f}, 1.0f, true}, // 12: French Horn (far left back)
        {{3.5f, 0, 1.5f}, 1.0f, true},  // 13: Tuba (far right back)
        {{-1, 0, 2}, 1.0f, true},      // 14: Harp (left back)
        {{1, 0, 2}, 1.0f, true}        // 15: Timpani (right back)
    };

    // 室内乐布局（更紧凑）
    // Chamber music layout (more compact)
    static const OrchestraChannelPosition ChamberLayout[16] = {
        {{0, 0, -1}, 1.0f, true},      // 0: Piano (center)
        {{-0.8f, 0, -0.5f}, 1.0f, true}, // 1: Violin I
        {{0.8f, 0, -0.5f}, 1.0f, true},  // 2: Violin II
        {{-1.2f, 0, 0}, 1.0f, true},   // 3: Viola
        {{1.2f, 0, 0}, 1.0f, true},    // 4: Cello
        {{0, 0, 0.5f}, 1.0f, true},    // 5: Bass
        {{-1.5f, 0, -1}, 1.0f, true},  // 6: Flute
        {{1.5f, 0, -1}, 1.0f, true},   // 7: Oboe
        {{-1.8f, 0, 0}, 1.0f, true},   // 8: Clarinet
        {{1.8f, 0, 0}, 1.0f, true},    // 9: Drums
        {{-1.5f, 0, 0.5f}, 1.0f, true}, // 10: Trumpet
        {{1.5f, 0, 0.5f}, 1.0f, true},  // 11: Trombone
        {{-2, 0, 1}, 1.0f, true},      // 12: French Horn
        {{2, 0, 1}, 1.0f, true},       // 13: Tuba
        {{-0.5f, 0, 1}, 1.0f, true},   // 14: Harp
        {{0.5f, 0, 1}, 1.0f, true}     // 15: Timpani
    };

    // 爵士乐布局
    // Jazz ensemble layout
    static const OrchestraChannelPosition JazzLayout[16] = {
        {{0, 0, 0}, 1.0f, true},       // 0: Piano (center)
        {{-1, 0, -0.5f}, 1.0f, true},  // 1: Saxophone (left)
        {{1, 0, -0.5f}, 1.0f, true},   // 2: Trumpet (right)
        {{-1.5f, 0, 0}, 1.0f, true},   // 3: Trombone (left)
        {{0, 0, 0.5f}, 1.0f, true},    // 4: Bass (center back)
        {{1.5f, 0, 0}, 1.0f, true},    // 5: Guitar (right)
        {{-2, 0, 0.5f}, 1.0f, true},   // 6: Clarinet (left back)
        {{2, 0, 0.5f}, 1.0f, true},    // 7: Vibes (right back)
        {{-0.5f, 0, -1}, 1.0f, true},  // 8: Alto Sax (left front)
        {{0.5f, 0, 1}, 1.0f, true},    // 9: Drums (center back)
        {{0, 0, -1.5f}, 1.0f, true},   // 10: Vocals (front center)
        {{-1, 0, 1}, 1.0f, true},      // 11: Flugelhorn (left back)
        {{1, 0, 1}, 1.0f, true},       // 12: French Horn (right back)
        {{-2.5f, 0, 0}, 1.0f, true},   // 13: Flute (far left)
        {{2.5f, 0, 0}, 1.0f, true},    // 14: Organ (far right)
        {{0, 0, 1.5f}, 1.0f, true}     // 15: Percussion (far back)
    };

    // 摇滚乐队布局
    // Rock band layout
    static const OrchestraChannelPosition RockLayout[16] = {
        {{0, 0, -1}, 1.0f, true},      // 0: Keyboard (front center)
        {{-1, 0, 0}, 1.0f, true},      // 1: Lead Guitar (left)
        {{1, 0, 0}, 1.0f, true},       // 2: Rhythm Guitar (right)
        {{-1.5f, 0, 0.5f}, 1.0f, true}, // 3: Bass (left back)
        {{0, 0, 1}, 1.0f, true},       // 4: Drums (center back)
        {{-0.5f, 0, -1.5f}, 1.0f, true}, // 5: Lead Vocals (front left)
        {{0.5f, 0, -1.5f}, 1.0f, true},  // 6: Backing Vocals (front right)
        {{-2, 0, 0}, 1.0f, true},      // 7: Synth (far left)
        {{2, 0, 0}, 1.0f, true},       // 8: Synth Pad (far right)
        {{1.5f, 0, 0.5f}, 1.0f, true}, // 9: Percussion (right back)
        {{-2.5f, 0, -0.5f}, 1.0f, true}, // 10: Effects (far left)
        {{2.5f, 0, -0.5f}, 1.0f, true},  // 11: Samples (far right)
        {{-1, 0, 1.5f}, 1.0f, true},   // 12: Tom-toms (left far back)
        {{1, 0, 1.5f}, 1.0f, true},    // 13: Cymbals (right far back)
        {{0, 0, 2}, 1.0f, true},       // 14: Kick Drum (far back)
        {{0, 1, 0}, 1.0f, true}        // 15: Overhead (above)
    };

    void MIDIOrchestraPlayer::InitPrivate()
    {
        if(!alGenSources)
        {
            LogError(OS_TEXT("OpenAL/EFX not initialized!"));
            return;
        }

        auto_gain.open=false;

        audio_ptr=nullptr;

        audio_data=nullptr;
        audio_data_size=0;

        audio_buffer_size=0;
        for(int i=0;i<MAX_MIDI_CHANNELS;i++)
        {
            channel_buffers[i]=nullptr;
            sources[i]=nullptr;
        }

        decode=nullptr;
        midi_config=nullptr;
        midi_channels=nullptr;
        
        audio_manager=nullptr;

        state=MIDIOrchestraState::None;

        orchestra_center=ZeroVector3f;
        orchestra_scale=1.0f;

        current_layout=OrchestraLayout::Standard;

        // Initialize all channel positions with standard layout
        for(int i=0;i<MAX_MIDI_CHANNELS;i++)
        {
            channel_positions[i]=StandardOrchestraLayout[i];
        }

        play=false;
        pause_state=false;
    }

    void MIDIOrchestraPlayer::InitializeChannelSources()
    {
        for(int i=0;i<MAX_MIDI_CHANNELS;i++)
        {
            sources[i]=new AudioSource;
            if(!sources[i])
            {
                LogError(OS_TEXT("Failed to create AudioSource for channel ") + OSString(i));
                continue;
            }

            // Generate 3 buffers for triple buffering
            alGenBuffers(3, buffers[i]);

            // Apply initial position and settings
            sources[i]->SetLoop(false);
            sources[i]->SetPosition(channel_positions[i].position + orchestra_center);
            sources[i]->SetGain(channel_positions[i].gain);

            // Set reasonable 3D audio parameters
            sources[i]->SetDistance(1.0f,50.0f);   // Maximum distance
            sources[i]->SetRolloffFactor(1.0f);    // Rolloff factor
        }
    }

    void MIDIOrchestraPlayer::ReleaseChannelSources()
    {
        for(int i=0;i<MAX_MIDI_CHANNELS;i++)
        {
            if(sources[i])
            {
                sources[i]->Stop();
                alDeleteBuffers(3, buffers[i]);
                delete sources[i];
                sources[i]=nullptr;
            }

            if(channel_buffers[i])
            {
                delete[] channel_buffers[i];
                channel_buffers[i]=nullptr;
            }
        }
    }

    void MIDIOrchestraPlayer::ApplyLayoutPreset(OrchestraLayout layout)
    {
        const OrchestraChannelPosition *preset=nullptr;

        switch(layout)
        {
            case OrchestraLayout::Standard:  preset=StandardOrchestraLayout; break;
            case OrchestraLayout::Chamber:   preset=ChamberLayout; break;
            case OrchestraLayout::Jazz:      preset=JazzLayout; break;
            case OrchestraLayout::Rock:      preset=RockLayout; break;
            case OrchestraLayout::Custom:    return; // Don't override custom positions
        }

        if(preset)
        {
            for(int i=0;i<MAX_MIDI_CHANNELS;i++)
            {
                channel_positions[i]=preset[i];
                if(sources[i])
                {
                    sources[i]->SetPosition((channel_positions[i].position * orchestra_scale) + orchestra_center);
                    sources[i]->SetGain(channel_positions[i].gain);
                }
            }
        }

        current_layout=layout;
    }

    MIDIOrchestraPlayer::MIDIOrchestraPlayer()
    {
        InitPrivate();
    }

    MIDIOrchestraPlayer::~MIDIOrchestraPlayer()
    {
        Close();

        SAFE_CLEAR(decode);

        ReleaseChannelSources();
    }

    bool MIDIOrchestraPlayer::LoadMIDI(const os_char *filename)
    {
        // MIDIOrchestraPlayer requires FluidSynth for optimal per-channel rendering
        // FluidSynth provides the best audio quality and native multi-channel support
        const os_char *plugin_name=OS_TEXT("FluidSynth");

        decode=new AudioPlugInInterface;

        if (!GetAudioInterface(plugin_name, decode, nullptr))
        {
            delete decode;
            decode=nullptr;

            LogError(OS_TEXT("Failed to load FluidSynth plugin - MIDIOrchestraPlayer requires FluidSynth for optimal multi-channel MIDI rendering"));
            LogError(OS_TEXT("Please ensure FluidSynth plugin (Audio.FluidSynth) is installed"));
            return(false);
        }

        // Get MIDI config interface
        midi_config=GetAudioMidiInterface(decode);
        if(!midi_config)
        {
            LogWarning(OS_TEXT("MIDI config interface not available"));
        }

        // Get MIDI channel interface - required for per-channel control
        midi_channels=GetAudioMidiChannelInterface(decode);
        if(!midi_channels)
        {
            LogError(OS_TEXT("MIDI channel interface not available - FluidSynth plugin may be outdated"));
            LogError(OS_TEXT("MIDIOrchestraPlayer requires AudioMidiChannelInterface for per-channel rendering"));
            return(false);
        }

        {
            double total_time=0;
            audio_ptr=decode->Open(audio_data,audio_data_size,&format,&rate,&total_time);

            if(!audio_ptr)
            {
                LogError(OS_TEXT("Failed to open MIDI file: ") + OSString(filename));
                return(false);
            }

            audio_buffer_size=(AudioTime(format,rate)+9)/10;        // 1/10 second per buffer

            // Allocate buffers for each channel
            for(int i=0;i<MAX_MIDI_CHANNELS;i++)
            {
                if(channel_buffers[i])
                    delete[] channel_buffers[i];

                channel_buffers[i]=new char[audio_buffer_size];
            }

            // Initialize channel sources
            InitializeChannelSources();

            return(true);
        }
    }

    bool MIDIOrchestraPlayer::LoadMIDI(io::InputStream *stream,int size)
    {
        if(!alGenBuffers)return(false);
        if(!stream)return(false);
        if(size<=0)return(false);

        Close();

        audio_data=new ALbyte[size];

        stream->Read(audio_data,size);

        audio_data_size=size;

        return LoadMIDI(OS_TEXT(""));
    }

    bool MIDIOrchestraPlayer::Load(const os_char *filename)
    {
        if(!alGenBuffers)return(false);
        if(!filename||!*filename)return(false);

        io::FileInputStream fis;

        if(!fis.Open(filename))
            return(false);

        Close();

        audio_data_size=fis.GetSize();
        audio_data=new ALbyte[audio_data_size];

        fis.Read(audio_data,audio_data_size);

        return LoadMIDI(filename);
    }

    bool MIDIOrchestraPlayer::Load(io::InputStream *stream,int size)
    {
        if(size==-1)
            size=stream->Available();

        return LoadMIDI(stream,size);
    }

    bool MIDIOrchestraPlayer::ReadChannelData(int channel, ALuint buffer_id)
    {
        if(!midi_channels||!channel_buffers[channel])
            return false;

        if(channel<0||channel>=MAX_MIDI_CHANNELS)
            return false;

        if(!channel_positions[channel].enabled)
            return false;

        // Decode single channel
        int bytes_read=midi_channels->DecodeChannel(audio_ptr, channel, 
                                                     channel_buffers[channel], 
                                                     audio_buffer_size);

        if(bytes_read<=0)
            return false;

        // Upload to OpenAL buffer
        alBufferData(buffer_id, format, channel_buffers[channel], bytes_read, rate);

        return true;
    }

    bool MIDIOrchestraPlayer::UpdateAllChannelBuffers()
    {
        for(int ch=0;ch<MAX_MIDI_CHANNELS;ch++)
        {
            if(!sources[ch]||!channel_positions[ch].enabled)
                continue;

            ALint processed=0;
            alGetSourcei(sources[ch]->GetIndex(), AL_BUFFERS_PROCESSED, &processed);

            while(processed--)
            {
                ALuint buffer_id;
                alSourceUnqueueBuffers(sources[ch]->GetIndex(), 1, &buffer_id);

                if(ReadChannelData(ch, buffer_id))
                {
                    alSourceQueueBuffers(sources[ch]->GetIndex(), 1, &buffer_id);
                }
            }
        }

        return true;
    }

    void MIDIOrchestraPlayer::ClearAllBuffers()
    {
        for(int ch=0;ch<MAX_MIDI_CHANNELS;ch++)
        {
            if(!sources[ch])continue;

            sources[ch]->Stop();

            ALint queued=0;
            alGetSourcei(sources[ch]->GetIndex(), AL_BUFFERS_QUEUED, &queued);

            while(queued--)
            {
                ALuint buffer_id;
                alSourceUnqueueBuffers(sources[ch]->GetIndex(), 1, &buffer_id);
            }
        }
    }

    bool MIDIOrchestraPlayer::PlaybackAllChannels()
    {
        // Initial fill of all channel buffers
        for(int ch=0;ch<MAX_MIDI_CHANNELS;ch++)
        {
            if(!sources[ch]||!channel_positions[ch].enabled)
                continue;

            // Fill 3 buffers for each channel
            for(int buf=0;buf<3;buf++)
            {
                if(!ReadChannelData(ch, buffers[ch][buf]))
                {
                    // This channel has no data yet or is inactive
                    break;
                }

                alSourceQueueBuffers(sources[ch]->GetIndex(), 1, &buffers[ch][buf]);
            }
        }

        // Start playback on all sources simultaneously
        for(int ch=0;ch<MAX_MIDI_CHANNELS;ch++)
        {
            if(!sources[ch]||!channel_positions[ch].enabled)
                continue;

            ALint queued=0;
            alGetSourcei(sources[ch]->GetIndex(), AL_BUFFERS_QUEUED, &queued);

            if(queued>0)
            {
                sources[ch]->Play();
            }
        }

        return true;
    }

    bool MIDIOrchestraPlayer::Execute()
    {
        while(state!=MIDIOrchestraState::Exit)
        {
            if(state==MIDIOrchestraState::None)
            {
                SleepSecond(0.1);
                continue;
            }

            if(state==MIDIOrchestraState::Pause)
            {
                for(int ch=0;ch<MAX_MIDI_CHANNELS;ch++)
                {
                    if(sources[ch])
                        sources[ch]->Pause();
                }

                while(state==MIDIOrchestraState::Pause)
                    SleepSecond(0.1);

                for(int ch=0;ch<MAX_MIDI_CHANNELS;ch++)
                {
                    if(sources[ch])
                        sources[ch]->Resume();
                }

                continue;
            }

            // Playing state
            UpdateAllChannelBuffers();

            // Apply auto gain if enabled
            if(auto_gain.open)
            {
                uint64 cur_time=GetMicroTime();
                double gap=double(cur_time-auto_gain.time)/HGL_MICRO_SEC_PER_SEC;

                if(gap>=auto_gain.gap)
                {
                    float new_gain=auto_gain.end.gain;
                    for(int ch=0;ch<MAX_MIDI_CHANNELS;ch++)
                    {
                        if(sources[ch])
                            sources[ch]->SetGain(new_gain * channel_positions[ch].gain);
                    }
                    auto_gain.open=false;
                }
                else
                {
                    float t=gap/auto_gain.gap;
                    float new_gain=auto_gain.start.gain+(auto_gain.end.gain-auto_gain.start.gain)*t;

                    for(int ch=0;ch<MAX_MIDI_CHANNELS;ch++)
                    {
                        if(sources[ch])
                            sources[ch]->SetGain(new_gain * channel_positions[ch].gain);
                    }
                }
            }

            // Check if any channel is still playing
            bool any_playing=false;
            for(int ch=0;ch<MAX_MIDI_CHANNELS;ch++)
            {
                if(sources[ch] && sources[ch]->IsPlaying())
                {
                    any_playing=true;
                    break;
                }
            }

            if(!any_playing && play)
            {
                // All channels finished
                if(play) // Check loop
                {
                    // Restart from beginning
                    if(decode&&audio_ptr)
                    {
                        decode->Close(audio_ptr);
                        double total_time=0;
                        audio_ptr=decode->Open(audio_data,audio_data_size,&format,&rate,&total_time);
                        
                        ClearAllBuffers();
                        PlaybackAllChannels();
                    }
                }
                else
                {
                    state=MIDIOrchestraState::None;
                }
            }

            SleepSecond(0.05); // 50ms update interval
        }

        return true;
    }

    void MIDIOrchestraPlayer::Play(bool loop)
    {
        if(state==MIDIOrchestraState::Play)return;

        play=loop;
        pause_state=false;

        if(!audio_ptr)return;

        ClearAllBuffers();

        PlaybackAllChannels();

        state=MIDIOrchestraState::Play;

        if(!IsRunning())
            Start();
    }

    void MIDIOrchestraPlayer::Pause()
    {
        if(state!=MIDIOrchestraState::Play)return;

        pause_state=true;
        state=MIDIOrchestraState::Pause;
    }

    void MIDIOrchestraPlayer::Resume()
    {
        if(state!=MIDIOrchestraState::Pause)return;

        pause_state=false;
        state=MIDIOrchestraState::Play;
    }

    void MIDIOrchestraPlayer::Stop()
    {
        if(state==MIDIOrchestraState::None)return;

        state=MIDIOrchestraState::None;
        pause_state=false;

        ClearAllBuffers();

        for(int ch=0;ch<MAX_MIDI_CHANNELS;ch++)
        {
            if(sources[ch])
                sources[ch]->Stop();
        }

        // Restart decoder
        if(decode&&audio_ptr)
        {
            decode->Close(audio_ptr);
            double total_time=0;
            audio_ptr=decode->Open(audio_data,audio_data_size,&format,&rate,&total_time);
        }
    }

    void MIDIOrchestraPlayer::Close()
    {
        Stop();

        state=MIDIOrchestraState::Exit;
        WaitStop();

        if(decode&&audio_ptr)
        {
            decode->Close(audio_ptr);
            audio_ptr=nullptr;
        }

        SAFE_CLEAR_ARRAY(audio_data);
    }

    // MIDI Configuration
    void MIDIOrchestraPlayer::SetSoundFont(const char *path)
    {
        if(midi_config)
            midi_config->SetSoundFont(path);
    }

    void MIDIOrchestraPlayer::SetBank(int bank_id)
    {
        if(midi_config)
            midi_config->SetBank(bank_id);
    }

    void MIDIOrchestraPlayer::SetSampleRate(int rate)
    {
        if(midi_config)
            midi_config->SetSampleRate(rate);
    }

    // 3D Spatial Layout
    void MIDIOrchestraPlayer::SetLayout(OrchestraLayout layout)
    {
        ApplyLayoutPreset(layout);
    }

    void MIDIOrchestraPlayer::SetOrchestraCenter(const Vector3f &center)
    {
        orchestra_center=center;

        // Update all source positions
        for(int i=0;i<MAX_MIDI_CHANNELS;i++)
        {
            if(sources[i])
            {
                sources[i]->SetPosition((channel_positions[i].position * orchestra_scale) + orchestra_center);
            }
        }
    }

    void MIDIOrchestraPlayer::SetOrchestraScale(float scale)
    {
        orchestra_scale=scale;

        // Update all source positions
        for(int i=0;i<MAX_MIDI_CHANNELS;i++)
        {
            if(sources[i])
            {
                sources[i]->SetPosition((channel_positions[i].position * orchestra_scale) + orchestra_center);
            }
        }
    }

    void MIDIOrchestraPlayer::SetChannelPosition(int channel, const Vector3f &position)
    {
        if(channel<0||channel>=MAX_MIDI_CHANNELS)return;

        channel_positions[channel].position=position;
        current_layout=OrchestraLayout::Custom;

        if(sources[channel])
        {
            sources[channel]->SetPosition((position * orchestra_scale) + orchestra_center);
        }
    }

    Vector3f MIDIOrchestraPlayer::GetChannelPosition(int channel) const
    {
        if(channel<0||channel>=MAX_MIDI_CHANNELS)
            return Vector3f(0,0,0);

        return channel_positions[channel].position;
    }

    void MIDIOrchestraPlayer::SetChannelVolume(int channel, float volume)
    {
        if(channel<0||channel>=MAX_MIDI_CHANNELS)return;

        channel_positions[channel].gain=volume;

        if(sources[channel])
        {
            sources[channel]->SetGain(volume);
        }

        if(midi_channels)
        {
            midi_channels->SetChannelVolume(channel, int(volume * 127));
        }
    }

    void MIDIOrchestraPlayer::SetChannelEnabled(int channel, bool enabled)
    {
        if(channel<0||channel>=MAX_MIDI_CHANNELS)return;

        channel_positions[channel].enabled=enabled;

        if(sources[channel])
        {
            if(!enabled)
                sources[channel]->Stop();
        }
    }

    AudioSource* MIDIOrchestraPlayer::GetChannelSource(int channel)
    {
        if(channel<0||channel>=MAX_MIDI_CHANNELS)return nullptr;

        return sources[channel];
    }

    // Channel Control
    void MIDIOrchestraPlayer::SetChannelProgram(int channel, int program)
    {
        if(midi_channels)
            midi_channels->SetChannelProgram(channel, program);
    }

    void MIDIOrchestraPlayer::MuteChannel(int channel, bool mute)
    {
        if(midi_channels)
            midi_channels->SetChannelMute(channel, mute);

        SetChannelEnabled(channel, !mute);
    }

    void MIDIOrchestraPlayer::SoloChannel(int channel, bool solo)
    {
        if(midi_channels)
            midi_channels->SetChannelSolo(channel, solo);

        if(solo)
        {
            // Mute all other channels
            for(int i=0;i<MAX_MIDI_CHANNELS;i++)
            {
                if(i!=channel)
                    SetChannelEnabled(i, false);
            }
            SetChannelEnabled(channel, true);
        }
        else
        {
            // Unmute all channels
            for(int i=0;i<MAX_MIDI_CHANNELS;i++)
            {
                SetChannelEnabled(i, true);
            }
        }
    }

    const int MIDIOrchestraPlayer::GetChannelCount()
    {
        if(midi_channels)
            return midi_channels->GetChannelCount();

        return 16; // Standard MIDI channel count
    }

    const MidiChannelInfo MIDIOrchestraPlayer::GetChannelInfo(int channel)
    {
        MidiChannelInfo info={0};

        if(midi_channels)
        {
            midi_channels->GetChannelInfo(channel, &info);
        }

        return info;
    }

    // Auto Gain
    void MIDIOrchestraPlayer::AutoGain(float start_gain,float gap,float end_gain)
    {
        auto_gain.open=true;
        auto_gain.time=GetMicroTime();
        auto_gain.gap=gap;
        auto_gain.start.gain=start_gain;
        auto_gain.end.gain=end_gain;

        // Apply start gain to all channels
        for(int ch=0;ch<MAX_MIDI_CHANNELS;ch++)
        {
            if(sources[ch])
            {
                sources[ch]->SetGain(start_gain * channel_positions[ch].gain);
            }
        }
    }
}//namespace hgl
