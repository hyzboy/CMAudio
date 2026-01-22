#define __MAKE_PLUGIN__

#include<hgl/plugin/PlugIn.h>
#include<hgl/plugin/PlugInInterface.h>
#include<malloc.h>
#include<string.h>
#include<hgl/al/al.h>

#include<fluidsynth.h>

using namespace hgl;
using namespace openal;

// FluidSynth global state
static fluid_settings_t* fluid_settings = nullptr;
static fluid_synth_t* fluid_synth = nullptr;
static bool fluidsynth_initialized = false;

// Configuration state
static char custom_soundfont_path[512] = {0};
static float global_volume = 1.0f;
static int sample_rate = 44100;
static int polyphony = 256;
static bool reverb_enabled = true;
static bool chorus_enabled = true;

// Get soundfont path from custom setting, environment, or use default
static const char* GetSoundFontPath()
{
    // Priority 1: Custom path set via API
    if (custom_soundfont_path[0] != '\0')
        return custom_soundfont_path;
    
    // Priority 2: Environment variable
    const char* sf_path = getenv("FLUIDSYNTH_SF2");
    if (sf_path && *sf_path)
        return sf_path;
    
#ifdef _WIN32
    return "C:\\soundfonts\\default.sf2";  // Windows default
#else
    // Common soundfont locations on Linux
    // Try FluidR3_GM.sf2, GeneralUser_GS.sf2, etc.
    return "/usr/share/sounds/sf2/FluidR3_GM.sf2";
#endif
}

static bool InitFluidSynth()
{
    if (fluidsynth_initialized)
        return true;
        
    fluid_settings = new_fluid_settings();
    if (!fluid_settings)
        return false;
    
    // Configure settings using configuration state
    fluid_settings_setnum(fluid_settings, "synth.sample-rate", (double)sample_rate);
    fluid_settings_setint(fluid_settings, "synth.polyphony", polyphony);
    fluid_settings_setint(fluid_settings, "synth.reverb.active", reverb_enabled ? 1 : 0);
    fluid_settings_setint(fluid_settings, "synth.chorus.active", chorus_enabled ? 1 : 0);
    fluid_settings_setnum(fluid_settings, "synth.gain", (double)global_volume);
    
    fluid_synth = new_fluid_synth(fluid_settings);
    if (!fluid_synth)
    {
        delete_fluid_settings(fluid_settings);
        fluid_settings = nullptr;
        return false;
    }
    
    // Load soundfont
    const char* sf_path = GetSoundFontPath();
    if (fluid_synth_sfload(fluid_synth, sf_path, 1) == FLUID_FAILED)
    {
        // Try alternative common paths
        const char* alt_paths[] = {
            "/usr/share/sounds/sf2/default.sf2",
            "/usr/share/soundfonts/default.sf2",
            "/usr/share/sounds/sf2/FluidR3_GM.sf2",
            "/usr/share/sounds/sf2/GeneralUser_GS.sf2",
            nullptr
        };
        
        bool loaded = false;
        for (int i = 0; alt_paths[i] != nullptr; i++)
        {
            if (fluid_synth_sfload(fluid_synth, alt_paths[i], 1) != FLUID_FAILED)
            {
                loaded = true;
                break;
            }
        }
        
        if (!loaded)
        {
            delete_fluid_synth(fluid_synth);
            delete_fluid_settings(fluid_settings);
            fluid_synth = nullptr;
            fluid_settings = nullptr;
            return false;
        }
    }
    
    fluidsynth_initialized = true;
    return true;
}

ALvoid LoadMIDI(ALbyte *memory, ALsizei memory_size, ALenum *format, ALvoid **data, ALsizei *size, ALsizei *freq, ALboolean *loop)
{
    if (!InitFluidSynth())
        return;

    // Create a player for this MIDI data
    fluid_player_t* player = new_fluid_player(fluid_synth);
    if (!player)
        return;

    // Load MIDI from memory
    if (fluid_player_add_mem(player, memory, memory_size) == FLUID_FAILED)
    {
        delete_fluid_player(player);
        return;
    }

    // FluidSynth outputs stereo 16-bit
    *format = AL_FORMAT_STEREO16;
    *freq = sample_rate;

    // Get total playing time (in milliseconds)
    int total_ticks = fluid_player_get_total_ticks(player);
    int tempo = fluid_player_get_bpm(player);
    double total_time_sec = (double)total_ticks * 60.0 / (tempo * 1000.0);
    
    size_t total_samples = (size_t)(total_time_sec * sample_rate);
    const size_t total_stereo_samples = total_samples * 2; // stereo
    const size_t pcm_total_bytes = total_stereo_samples * sizeof(int16_t);

    int16_t *ptr = new int16_t[total_stereo_samples];
    size_t out_size = 0;

    // Start playback
    fluid_player_play(player);

    // Render audio
    const size_t buffer_size = 4096; // samples per channel
    int16_t render_buffer[buffer_size * 2]; // stereo
    
    while (fluid_player_get_status(player) == FLUID_PLAYER_PLAYING && out_size < pcm_total_bytes)
    {
        // Render audio chunk
        if (fluid_synth_write_s16(fluid_synth, buffer_size, render_buffer, 0, 2, 
                                  render_buffer, 1, 2) == FLUID_FAILED)
            break;
        
        size_t bytes_to_copy = buffer_size * 2 * sizeof(int16_t);
        if (out_size + bytes_to_copy > pcm_total_bytes)
            bytes_to_copy = pcm_total_bytes - out_size;
            
        memcpy((char*)ptr + out_size, render_buffer, bytes_to_copy);
        out_size += bytes_to_copy;
    }

    *size = (int)out_size;
    *data = ptr;

    delete_fluid_player(player);
}

void ClearMIDI(ALenum, ALvoid *data, ALsizei, ALsizei)
{
    if (data)
        delete[] (int16_t *)data;
}

//--------------------------------------------------------------------------------------------------
struct MidiStream
{
    fluid_player_t* player;
    unsigned char *midi_data;
    size_t midi_size;
    unsigned long sample_rate;
    bool playing;
};

void *OpenMIDI(ALbyte *memory, ALsizei memory_size, ALenum *format, ALsizei *rate, double *total_time)
{
    if (!InitFluidSynth())
        return nullptr;

    MidiStream *stream = new MidiStream;
    
    // Store MIDI data for potential restart
    stream->midi_data = (unsigned char*)memory;
    stream->midi_size = memory_size;
    stream->sample_rate = sample_rate;
    stream->playing = false;
    
    stream->player = new_fluid_player(fluid_synth);
    if (!stream->player)
    {
        delete stream;
        return nullptr;
    }

    // Load MIDI from memory
    if (fluid_player_add_mem(stream->player, stream->midi_data, stream->midi_size) == FLUID_FAILED)
    {
        delete_fluid_player(stream->player);
        delete stream;
        return nullptr;
    }

    // FluidSynth outputs stereo 16-bit
    *format = AL_FORMAT_STEREO16;
    *rate = stream->sample_rate;

    // Get total time
    int total_ticks = fluid_player_get_total_ticks(stream->player);
    int tempo = fluid_player_get_bpm(stream->player);
    *total_time = (double)total_ticks * 60.0 / (tempo * 1000.0);
    
    // Start playback
    fluid_player_play(stream->player);
    stream->playing = true;

    return stream;
}

void CloseMIDI(void *ptr)
{
    MidiStream *stream = (MidiStream *)ptr;
    
    if (stream->player)
    {
        fluid_player_stop(stream->player);
        delete_fluid_player(stream->player);
    }
    
    delete stream;
}

uint ReadMIDI(void *ptr, char *data, uint buf_max)
{
    MidiStream *stream = (MidiStream *)ptr;
    
    if (!stream || !stream->player || !stream->playing)
        return 0;
    
    // Check if still playing
    if (fluid_player_get_status(stream->player) != FLUID_PLAYER_PLAYING)
    {
        stream->playing = false;
        return 0;
    }
    
    // Render audio
    size_t samples = buf_max / (2 * sizeof(int16_t)); // stereo 16-bit
    if (fluid_synth_write_s16(fluid_synth, samples, (int16_t*)data, 0, 2, 
                              (int16_t*)data, 1, 2) == FLUID_FAILED)
        return 0;

    return buf_max;
}

void RestartMIDI(void *ptr)
{
    MidiStream *stream = (MidiStream *)ptr;
    
    if (!stream)
        return;
    
    // Stop and delete current player
    if (stream->player)
    {
        fluid_player_stop(stream->player);
        delete_fluid_player(stream->player);
        stream->player = nullptr;
    }
    
    // Create new player and reload
    stream->player = new_fluid_player(fluid_synth);
    if (stream->player)
    {
        if (fluid_player_add_mem(stream->player, stream->midi_data, stream->midi_size) == FLUID_OK)
        {
            fluid_player_play(stream->player);
            stream->playing = true;
        }
        else
        {
            delete_fluid_player(stream->player);
            stream->player = nullptr;
            stream->playing = false;
        }
    }
    
    // Note: If reload fails, player will be nullptr and subsequent ReadMIDI calls will return 0
}

//--------------------------------------------------------------------------------------------------
// MIDI Configuration Interface
//--------------------------------------------------------------------------------------------------

void SetSoundFont(const char* path)
{
    if (path && path[0] != '\0')
    {
        strncpy(custom_soundfont_path, path, sizeof(custom_soundfont_path) - 1);
        custom_soundfont_path[sizeof(custom_soundfont_path) - 1] = '\0';
        
        // If already initialized, reload the soundfont
        if (fluidsynth_initialized && fluid_synth)
        {
            fluid_synth_sfload(fluid_synth, path, 1);
        }
    }
}

void SetBankID(int bank_id)
{
    // FluidSynth doesn't use bank IDs like chip emulators
    // This is a no-op for FluidSynth
}

void SetVolume(float volume)
{
    global_volume = volume;
    if (fluidsynth_initialized && fluid_settings)
    {
        fluid_settings_setnum(fluid_settings, "synth.gain", (double)volume);
    }
}

void SetSampleRate(int rate)
{
    sample_rate = rate;
    // Requires reinitialization to take effect
}

void SetPolyphony(int poly)
{
    polyphony = poly;
    if (fluidsynth_initialized && fluid_synth)
    {
        fluid_synth_set_polyphony(fluid_synth, poly);
    }
}

void SetChipCount(int count)
{
    // FluidSynth doesn't use chip emulation
    // This is a no-op for FluidSynth
}

void EnableReverb(bool enable)
{
    reverb_enabled = enable;
    if (fluidsynth_initialized && fluid_synth)
    {
        fluid_synth_set_reverb_on(fluid_synth, enable ? 1 : 0);
    }
}

void EnableChorus(bool enable)
{
    chorus_enabled = enable;
    if (fluidsynth_initialized && fluid_synth)
    {
        fluid_synth_set_chorus_on(fluid_synth, enable ? 1 : 0);
    }
}

const char* GetVersion()
{
    return "FluidSynth 2.x MIDI Synthesizer";
}

const char* GetDefaultBank()
{
    return GetSoundFontPath();
}

//--------------------------------------------------------------------------------------------------
// MIDI Channel Interface
//--------------------------------------------------------------------------------------------------

// Channel state tracking
struct ChannelState
{
    int program;
    int bank;
    float volume;
    float pan;
    bool muted;
    bool solo;
};

static ChannelState channel_states[16] = {0};
static bool any_solo_active = false;

int GetChannelCount()
{
    return 16;  // Standard MIDI has 16 channels
}

bool GetChannelInfo(int channel, void* info_ptr)
{
    if (channel < 0 || channel >= 16 || !info_ptr)
        return false;
    
    // Define local structure matching the interface
    struct MidiChannelInfo
    {
        int channel;
        int program;
        int bank;
        float volume;
        float pan;
        bool muted;
        bool solo;
        int note_count;
        const char* instrument_name;
    };
    
    MidiChannelInfo* info = (MidiChannelInfo*)info_ptr;
    info->channel = channel;
    info->program = channel_states[channel].program;
    info->bank = channel_states[channel].bank;
    info->volume = channel_states[channel].volume;
    info->pan = channel_states[channel].pan;
    info->muted = channel_states[channel].muted;
    info->solo = channel_states[channel].solo;
    info->note_count = 0;  // FluidSynth doesn't easily expose active note count
    info->instrument_name = nullptr;  // Would need GM instrument name lookup
    
    return true;
}

void SetChannelProgram(int channel, int program)
{
    if (channel < 0 || channel >= 16 || program < 0 || program > 127)
        return;
    
    channel_states[channel].program = program;
    
    if (fluidsynth_initialized && fluid_synth)
    {
        fluid_synth_program_change(fluid_synth, channel, program);
    }
}

void SetChannelBank(int channel, int bank)
{
    if (channel < 0 || channel >= 16)
        return;
    
    channel_states[channel].bank = bank;
    
    if (fluidsynth_initialized && fluid_synth)
    {
        // Set bank MSB and LSB
        fluid_synth_bank_select(fluid_synth, channel, bank);
    }
}

void SetChannelVolume(int channel, float volume)
{
    if (channel < 0 || channel >= 16)
        return;
    
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    
    channel_states[channel].volume = volume;
    
    if (fluidsynth_initialized && fluid_synth)
    {
        // MIDI volume CC is 0-127
        int midi_volume = (int)(volume * 127.0f);
        fluid_synth_cc(fluid_synth, channel, 7, midi_volume);
    }
}

void SetChannelPan(int channel, float pan)
{
    if (channel < 0 || channel >= 16)
        return;
    
    if (pan < -1.0f) pan = -1.0f;
    if (pan > 1.0f) pan = 1.0f;
    
    channel_states[channel].pan = pan;
    
    if (fluidsynth_initialized && fluid_synth)
    {
        // MIDI pan CC is 0-127, with 64 being center
        int midi_pan = (int)((pan + 1.0f) * 63.5f);
        fluid_synth_cc(fluid_synth, channel, 10, midi_pan);
    }
}

void MuteChannel(int channel, bool mute)
{
    if (channel < 0 || channel >= 16)
        return;
    
    channel_states[channel].muted = mute;
    
    if (mute)
    {
        SetChannelVolume(channel, 0.0f);
    }
    else
    {
        SetChannelVolume(channel, channel_states[channel].volume);
    }
}

void SoloChannel(int channel, bool solo)
{
    if (channel < 0 || channel >= 16)
        return;
    
    channel_states[channel].solo = solo;
    
    // Update solo state
    any_solo_active = false;
    for (int i = 0; i < 16; i++)
    {
        if (channel_states[i].solo)
        {
            any_solo_active = true;
            break;
        }
    }
    
    // Apply muting based on solo state
    for (int i = 0; i < 16; i++)
    {
        if (any_solo_active)
        {
            // If any channel is soloed, mute all non-soloed channels
            if (!channel_states[i].solo && !channel_states[i].muted)
            {
                SetChannelVolume(i, 0.0f);
            }
            else if (channel_states[i].solo)
            {
                SetChannelVolume(i, channel_states[i].volume);
            }
        }
        else
        {
            // No solo active, restore original volumes
            if (!channel_states[i].muted)
            {
                SetChannelVolume(i, channel_states[i].volume);
            }
        }
    }
}

// Multi-channel decode support
// Note: FluidSynth synthesizes all channels together, so true per-channel
// separation requires rendering multiple times with channels muted/unmuted
void* OpenMultiChannel(ALbyte *data, ALsizei size, ALenum *format, ALsizei *freq, double *time)
{
    // For multi-channel decode, use regular Open but mark it
    return OpenMIDI(data, size, format, freq, time);
}

uint ReadChannel(void *stream, int channel, char *pcm_data, uint buf_size)
{
    if (!stream || channel < 0 || channel >= 16)
        return 0;
    
    // Mute all other channels temporarily
    bool original_mute[16];
    for (int i = 0; i < 16; i++)
    {
        original_mute[i] = channel_states[i].muted;
        if (i != channel)
        {
            MuteChannel(i, true);
        }
    }
    
    // Read the audio with only this channel active
    uint result = ReadMIDI(stream, pcm_data, buf_size);
    
    // Restore original mute states
    for (int i = 0; i < 16; i++)
    {
        channel_states[i].muted = original_mute[i];
        if (!original_mute[i])
        {
            SetChannelVolume(i, channel_states[i].volume);
        }
    }
    
    return result;
}

uint ReadChannels(void *stream, int *channels, int channel_count, char **pcm_data, uint buf_size)
{
    if (!stream || !channels || !pcm_data || channel_count <= 0)
        return 0;
    
    // For multiple channels, we need to render separately
    // This is expensive but accurate
    uint result = 0;
    for (int i = 0; i < channel_count; i++)
    {
        result = ReadChannel(stream, channels[i], pcm_data[i], buf_size);
        if (result == 0)
            break;
    }
    
    return result;
}

void CloseMultiChannel(void *stream)
{
    CloseMIDI(stream);
}

//--------------------------------------------------------------------------------------------------
struct OutInterface
{
    void (*Load)(ALbyte *, ALsizei, ALenum *, ALvoid **, ALsizei *, ALsizei *, ALboolean *);
    void (*Clear)(ALenum, ALvoid *, ALsizei, ALsizei);

    void *(*Open)(ALbyte *, ALsizei, ALenum *, ALsizei *, double *);
    void  (*Close)(void *);
    uint  (*Read)(void *, char *, uint);
    void  (*Restart)(void *);
};

static OutInterface out_interface =
{
    LoadMIDI,
    ClearMIDI,

    OpenMIDI,
    CloseMIDI,
    ReadMIDI,
    RestartMIDI
};

struct MidiConfigInterface
{
    void (*SetSoundFont)(const char *);
    void (*SetBankID)(int);
    void (*SetVolume)(float);
    void (*SetSampleRate)(int);
    void (*SetPolyphony)(int);
    void (*SetChipCount)(int);
    void (*EnableReverb)(bool);
    void (*EnableChorus)(bool);
    const char* (*GetVersion)();
    const char* (*GetDefaultBank)();
};

static MidiConfigInterface midi_config_interface =
{
    SetSoundFont,
    SetBankID,
    SetVolume,
    SetSampleRate,
    SetPolyphony,
    SetChipCount,
    EnableReverb,
    EnableChorus,
    GetVersion,
    GetDefaultBank
};

struct MidiChannelInterface
{
    int  (*GetChannelCount)();
    bool (*GetChannelInfo)(int channel, void*);
    void (*SetChannelProgram)(int channel, int program);
    void (*SetChannelBank)(int channel, int bank);
    void (*SetChannelVolume)(int channel, float volume);
    void (*SetChannelPan)(int channel, float pan);
    void (*MuteChannel)(int channel, bool mute);
    void (*SoloChannel)(int channel, bool solo);
    void* (*OpenMultiChannel)(ALbyte *data, ALsizei size, ALenum *format, ALsizei *freq, double *time);
    uint (*ReadChannel)(void *stream, int channel, char *pcm_data, uint buf_size);
    uint (*ReadChannels)(void *stream, int *channels, int channel_count, char **pcm_data, uint buf_size);
    void (*CloseMultiChannel)(void *stream);
};

static MidiChannelInterface midi_channel_interface =
{
    GetChannelCount,
    GetChannelInfo,
    SetChannelProgram,
    SetChannelBank,
    SetChannelVolume,
    SetChannelPan,
    MuteChannel,
    SoloChannel,
    OpenMultiChannel,
    ReadChannel,
    ReadChannels,
    CloseMultiChannel
};

//--------------------------------------------------------------------------------------------------
const u16char plugin_intro[] = U16_TEXT("MIDI 音频文件解码(FluidSynth)");

HGL_PLUGIN_FUNC uint32 GetPlugInVersion()
{
    return(2);        // Version 2 interface
}

u16char * GetPlugInIntro()
{
    return((u16char *)plugin_intro);
}

bool GetPlugInInterface(uint32 ver, void *data)
{
    if (ver == 2)
    {
        memcpy(data, &out_interface, sizeof(OutInterface));
        return true;
    }
    else if (ver == 4)
    {
        memcpy(data, &midi_config_interface, sizeof(MidiConfigInterface));
        return true;
    }
    else if (ver == 5)
    {
        memcpy(data, &midi_channel_interface, sizeof(MidiChannelInterface));
        return true;
    }
    
    return false;
}

static PlugInInterface pii =
{
    nullptr,

    GetPlugInVersion,
    GetPlugInIntro,

    GetPlugInInterface,
    nullptr,

    nullptr,
    nullptr
};

HGL_PLUGIN_FUNC PlugInInterface *InitPlugIn()
{
    return &pii;
}

// Plugin cleanup
HGL_PLUGIN_FUNC void ClosePlugIn()
{
    if (fluidsynth_initialized)
    {
        if (fluid_synth)
        {
            delete_fluid_synth(fluid_synth);
            fluid_synth = nullptr;
        }
        if (fluid_settings)
        {
            delete_fluid_settings(fluid_settings);
            fluid_settings = nullptr;
        }
        fluidsynth_initialized = false;
    }
}
//--------------------------------------------------------------------------------------------------
