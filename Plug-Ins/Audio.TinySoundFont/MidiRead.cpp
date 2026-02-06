#define __MAKE_PLUGIN__

#include<hgl/plugin/PlugIn.h>
#include<hgl/plugin/PlugInInterface.h>
#include<malloc.h>
#include<string.h>
#include<hgl/al/al.h>

#define TSF_IMPLEMENTATION
#include "tsf.h"

#define TML_IMPLEMENTATION
#include "tml.h"

using namespace hgl;
using namespace openal;

// TinySoundFont global state
static tsf* g_tsf = nullptr;
static bool tsf_initialized = false;

// Configuration state
static char custom_soundfont_path[512] = {0};
static float global_volume = 1.0f;
static int sample_rate = 44100;
static int polyphony = 64;

// Get soundfont path from custom setting, environment, or use default
static const char* GetSoundFontPath()
{
    // Priority 1: Custom path set via API
    if (custom_soundfont_path[0] != '\0')
        return custom_soundfont_path;
    
    // Priority 2: Environment variable
    const char* sf_path = getenv("TSF_SOUNDFONT");
    if (sf_path && *sf_path)
        return sf_path;
    
#ifdef _WIN32
    return "C:\\soundfonts\\default.sf2";  // Windows default
#else
    // Common soundfont locations on Linux
    return "/usr/share/sounds/sf2/FluidR3_GM.sf2";
#endif
}

static bool InitTinySoundFont()
{
    if (tsf_initialized && g_tsf)
        return true;
        
    const char* sf_path = GetSoundFontPath();
    g_tsf = tsf_load_filename(sf_path);
    
    if (!g_tsf)
    {
        // Try alternative common paths
        const char* alt_paths[] = {
            "/usr/share/sounds/sf2/default.sf2",
            "/usr/share/soundfonts/default.sf2",
            "/usr/share/sounds/sf2/FluidR3_GM.sf2",
            "/usr/share/sounds/sf2/GeneralUser_GS.sf2",
            nullptr
        };
        
        for (int i = 0; alt_paths[i] != nullptr; i++)
        {
            g_tsf = tsf_load_filename(alt_paths[i]);
            if (g_tsf)
                break;
        }
        
        if (!g_tsf)
            return false;
    }
    
    // Set output mode using configuration state
    tsf_set_output(g_tsf, TSF_STEREO_INTERLEAVED, sample_rate, 0.0f);
    tsf_set_volume(g_tsf, global_volume);
    
    tsf_initialized = true;
    return true;
}

ALvoid LoadMIDI(ALbyte *memory, ALsizei memory_size, ALenum *format, ALvoid **data, ALsizei *size, ALsizei *freq, ALboolean *loop)
{
    if (!InitTinySoundFont())
        return;

    // Load MIDI from memory
    tml_message* midi = tml_load_memory(memory, memory_size);
    if (!midi)
        return;

    // TinySoundFont outputs stereo 16-bit
    *format = AL_FORMAT_STEREO16;
    *freq = sample_rate;

    // Calculate total time in seconds
    double total_time = 0.0;
    for (tml_message* msg = midi; msg; msg = msg->next)
    {
        if (msg->time > total_time)
            total_time = msg->time;
    }
    total_time /= 1000.0; // Convert ms to seconds
    
    size_t total_samples = (size_t)(total_time * sample_rate);
    const size_t total_stereo_samples = total_samples * 2; // stereo
    const size_t pcm_total_bytes = total_stereo_samples * sizeof(int16_t);

    int16_t *ptr = new int16_t[total_stereo_samples];
    size_t out_size = 0;

    // Reset synthesizer
    tsf_reset(g_tsf);
    
    // Render MIDI to PCM
    tml_message* msg = midi;
    double current_time = 0.0;
    const size_t render_samples = 4096; // samples per channel per render
    
    while (out_size < pcm_total_bytes && msg)
    {
        // Process MIDI messages up to next render time
        double next_time = current_time + (render_samples / (double)sample_rate * 1000.0); // in ms
        
        while (msg && msg->time <= next_time)
        {
            switch (msg->type)
            {
                case TML_PROGRAM_CHANGE:
                    tsf_channel_set_presetnumber(g_tsf, msg->channel, msg->program, (msg->channel == 9));
                    break;
                case TML_NOTE_ON:
                    tsf_channel_note_on(g_tsf, msg->channel, msg->key, msg->velocity / 127.0f);
                    break;
                case TML_NOTE_OFF:
                    tsf_channel_note_off(g_tsf, msg->channel, msg->key);
                    break;
                case TML_PITCH_BEND:
                    tsf_channel_set_pitchwheel(g_tsf, msg->channel, msg->pitch_bend);
                    break;
                case TML_CONTROL_CHANGE:
                    tsf_channel_midi_control(g_tsf, msg->channel, msg->control, msg->control_value);
                    break;
            }
            msg = msg->next;
        }
        
        // Render audio
        size_t samples_to_render = render_samples;
        if (out_size + samples_to_render * 2 * sizeof(int16_t) > pcm_total_bytes)
            samples_to_render = (pcm_total_bytes - out_size) / (2 * sizeof(int16_t));
        
        if (samples_to_render > 0)
        {
            tsf_render_short(g_tsf, (short*)((char*)ptr + out_size), samples_to_render, 0);
            out_size += samples_to_render * 2 * sizeof(int16_t);
            current_time = next_time;
        }
        else
            break;
    }

    *size = (int)out_size;
    *data = ptr;

    tml_free(midi);
}

void ClearMIDI(ALenum, ALvoid *data, ALsizei, ALsizei)
{
    if (data)
        delete[] (int16_t *)data;
}

//--------------------------------------------------------------------------------------------------
struct MidiStream
{
    tml_message* midi;
    tml_message* current_msg;
    double current_time;
    unsigned char *midi_data;
    size_t midi_size;
    unsigned long sample_rate;
};

void *OpenMIDI(ALbyte *memory, ALsizei memory_size, ALenum *format, ALsizei *rate, double *total_time)
{
    if (!InitTinySoundFont())
        return nullptr;

    MidiStream *stream = new MidiStream;
    
    // Store MIDI data for potential restart
    stream->midi_data = (unsigned char*)memory;
    stream->midi_size = memory_size;
    stream->sample_rate = sample_rate;
    stream->current_time = 0.0;
    
    stream->midi = tml_load_memory(stream->midi_data, stream->midi_size);
    if (!stream->midi)
    {
        delete stream;
        return nullptr;
    }
    
    stream->current_msg = stream->midi;

    // TinySoundFont outputs stereo 16-bit
    *format = AL_FORMAT_STEREO16;
    *rate = stream->sample_rate;

    // Calculate total time
    double max_time = 0.0;
    for (tml_message* msg = stream->midi; msg; msg = msg->next)
    {
        if (msg->time > max_time)
            max_time = msg->time;
    }
    *total_time = max_time / 1000.0;
    
    // Reset synthesizer for this stream
    tsf_reset(g_tsf);

    return stream;
}

void CloseMIDI(void *ptr)
{
    MidiStream *stream = (MidiStream *)ptr;
    
    if (stream->midi)
        tml_free(stream->midi);
    
    delete stream;
}

uint ReadMIDI(void *ptr, char *data, uint buf_max)
{
    MidiStream *stream = (MidiStream *)ptr;
    
    if (!stream || !stream->midi)
        return 0;
    
    // Calculate samples to render
    size_t samples = buf_max / (2 * sizeof(int16_t)); // stereo 16-bit
    
    // Process MIDI messages
    double next_time = stream->current_time + (samples / (double)stream->sample_rate * 1000.0); // in ms
    
    while (stream->current_msg && stream->current_msg->time <= next_time)
    {
        switch (stream->current_msg->type)
        {
            case TML_PROGRAM_CHANGE:
                tsf_channel_set_presetnumber(g_tsf, stream->current_msg->channel, 
                                            stream->current_msg->program, 
                                            (stream->current_msg->channel == 9));
                break;
            case TML_NOTE_ON:
                tsf_channel_note_on(g_tsf, stream->current_msg->channel, 
                                   stream->current_msg->key, 
                                   stream->current_msg->velocity / 127.0f);
                break;
            case TML_NOTE_OFF:
                tsf_channel_note_off(g_tsf, stream->current_msg->channel, 
                                    stream->current_msg->key);
                break;
            case TML_PITCH_BEND:
                tsf_channel_set_pitchwheel(g_tsf, stream->current_msg->channel, 
                                          stream->current_msg->pitch_bend);
                break;
            case TML_CONTROL_CHANGE:
                tsf_channel_midi_control(g_tsf, stream->current_msg->channel, 
                                        stream->current_msg->control, 
                                        stream->current_msg->control_value);
                break;
        }
        stream->current_msg = stream->current_msg->next;
    }
    
    // Check if we've reached the end
    if (!stream->current_msg)
        return 0;
    
    // Render audio
    tsf_render_short(g_tsf, (short*)data, samples, 0);
    stream->current_time = next_time;

    return buf_max;
}

void RestartMIDI(void *ptr)
{
    MidiStream *stream = (MidiStream *)ptr;
    
    if (!stream)
        return;
    
    // Reset to beginning
    stream->current_msg = stream->midi;
    stream->current_time = 0.0;
    
    // Reset synthesizer
    tsf_reset(g_tsf);
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
        if (tsf_initialized && g_tsf)
        {
            tsf_close(g_tsf);
            g_tsf = tsf_load_filename(path);
            if (g_tsf)
            {
                tsf_set_output(g_tsf, TSF_STEREO_INTERLEAVED, sample_rate, 0.0f);
                tsf_set_volume(g_tsf, global_volume);
            }
            else
            {
                tsf_initialized = false;
            }
        }
    }
}

void SetBankID(int bank_id)
{
    // TinySoundFont doesn't use bank IDs like chip emulators
    // This is a no-op for TinySoundFont
}

void SetVolume(float volume)
{
    global_volume = volume;
    if (tsf_initialized && g_tsf)
    {
        tsf_set_volume(g_tsf, volume);
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
    // TinySoundFont manages polyphony automatically
}

void SetChipCount(int count)
{
    // TinySoundFont doesn't use chip emulation
    // This is a no-op for TinySoundFont
}

void EnableReverb(bool enable)
{
    // TinySoundFont doesn't have built-in reverb
    // This is a no-op for TinySoundFont
}

void EnableChorus(bool enable)
{
    // TinySoundFont doesn't have built-in chorus
    // This is a no-op for TinySoundFont
}

const char* GetVersionString()
{
    return "TinySoundFont MIDI Synthesizer";
}

const char* GetDefaultBank()
{
    return GetSoundFontPath();
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
    const char* (*GetVersionString)();
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
    GetVersionString,
    GetDefaultBank
};

//--------------------------------------------------------------------------------------------------
const u16char plugin_intro[] = U16_TEXT("MIDI 音频文件解码(TinySoundFont)");

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
    if (tsf_initialized && g_tsf)
    {
        tsf_close(g_tsf);
        g_tsf = nullptr;
        tsf_initialized = false;
    }
}
//--------------------------------------------------------------------------------------------------
