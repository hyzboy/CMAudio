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

// Get soundfont path from environment or use default
static const char* GetSoundFontPath()
{
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
    
    // Configure settings for real-time synthesis
    fluid_settings_setnum(fluid_settings, "synth.sample-rate", 44100.0);
    fluid_settings_setint(fluid_settings, "synth.polyphony", 256);
    fluid_settings_setint(fluid_settings, "synth.reverb.active", 1);
    fluid_settings_setint(fluid_settings, "synth.chorus.active", 1);
    
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
    *freq = 44100;

    // Get total playing time (in milliseconds)
    int total_ticks = fluid_player_get_total_ticks(player);
    int tempo = fluid_player_get_bpm(player);
    double total_time_sec = (double)total_ticks * 60.0 / (tempo * 1000.0);
    
    size_t total_samples = (size_t)(total_time_sec * 44100);
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
    stream->sample_rate = 44100;
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
    }
    else
        return false;

    return true;
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
