#define __MAKE_PLUGIN__

#include<hgl/plugin/PlugIn.h>
#include<hgl/plugin/PlugInInterface.h>
#include<malloc.h>
#include<string.h>
#include<hgl/al/al.h>

#include<timidity.h>

using namespace hgl;
using namespace openal;

// Timidity global initialization state
static bool timidity_initialized = false;

// Get config path from environment or use default
static const char* GetConfigPath()
{
    const char* config_path = getenv("TIMIDITY_CFG");
    if (config_path && *config_path)
        return config_path;
    
#ifdef _WIN32
    return "C:\\timidity\\timidity.cfg";  // Windows default
#else
    return "/etc/timidity/timidity.cfg";  // Unix/Linux default
#endif
}

ALvoid LoadMIDI(ALbyte *memory, ALsizei memory_size, ALenum *format, ALvoid **data, ALsizei *size, ALsizei *freq, ALboolean *loop)
{
    if (!timidity_initialized)
    {
        const char* config_path = GetConfigPath();
        if (mid_init(const_cast<char*>(config_path)) < 0)
        {
            return;  // Failed to initialize
        }
        timidity_initialized = true;
    }

    MidSong *song = mid_song_load_dls(memory, memory_size, 44100, MID_AUDIO_S16LSB, 2, 44100 * 2 * 2);
    if (!song)
        return;

    // Libtimidity outputs stereo 16-bit by default
    *format = AL_FORMAT_STEREO16;
    *freq = 44100;

    // Get song length in samples
    mid_song_start(song);
    size_t total_samples = mid_song_get_total_time(song) * 44100 / 1000; // Convert ms to samples
    const size_t total_stereo_samples = total_samples * 2; // stereo
    const size_t pcm_total_bytes = total_stereo_samples * sizeof(int16_t);

    int16_t *ptr = new int16_t[total_stereo_samples];
    int out_size = 0;
    int read_size;

    // Read all MIDI data converted to PCM
    while (out_size < (int)pcm_total_bytes)
    {
        read_size = mid_song_read_wave(song, (void*)((char*)ptr + out_size), pcm_total_bytes - out_size);
        
        if (read_size <= 0)
            break;
            
        out_size += read_size;
    }

    *size = out_size;
    *data = ptr;

    mid_song_free(song);
}

void ClearMIDI(ALenum, ALvoid *data, ALsizei, ALsizei)
{
    if (data)
        delete[] (int16_t *)data;
}

//--------------------------------------------------------------------------------------------------
struct MidiStream
{
    MidSong *song;
    unsigned char *midi_data;
    size_t midi_size;
    unsigned long sample_rate;
};

void *OpenMIDI(ALbyte *memory, ALsizei memory_size, ALenum *format, ALsizei *rate, double *total_time)
{
    if (!timidity_initialized)
    {
        const char* config_path = GetConfigPath();
        if (mid_init(const_cast<char*>(config_path)) < 0)
        {
            return nullptr;  // Failed to initialize
        }
        timidity_initialized = true;
    }

    MidiStream *stream = new MidiStream;
    
    // Store MIDI data for potential restart
    stream->midi_data = (unsigned char*)memory;
    stream->midi_size = memory_size;
    stream->sample_rate = 44100;
    
    stream->song = mid_song_load_dls(stream->midi_data, stream->midi_size, 
                                     stream->sample_rate, MID_AUDIO_S16LSB, 2, 
                                     stream->sample_rate * 2 * 2);
    if (!stream->song)
    {
        delete stream;
        return nullptr;
    }

    // Start the song
    mid_song_start(stream->song);

    // Libtimidity outputs stereo 16-bit
    *format = AL_FORMAT_STEREO16;
    *rate = stream->sample_rate;

    // Get total time in seconds
    *total_time = (double)mid_song_get_total_time(stream->song) / 1000.0;

    return stream;
}

void CloseMIDI(void *ptr)
{
    MidiStream *stream = (MidiStream *)ptr;
    
    if (stream->song)
        mid_song_free(stream->song);
    
    delete stream;
}

uint ReadMIDI(void *ptr, char *data, uint buf_max)
{
    MidiStream *stream = (MidiStream *)ptr;
    
    if (!stream || !stream->song)
        return 0;
    
    int result = mid_song_read_wave(stream->song, data, buf_max);

    if (result <= 0)
        return 0;

    return result;
}

void RestartMIDI(void *ptr)
{
    MidiStream *stream = (MidiStream *)ptr;
    
    if (!stream)
        return;
    
    // Free current song
    if (stream->song)
    {
        mid_song_free(stream->song);
        stream->song = nullptr;
    }
    
    // Reload and restart
    stream->song = mid_song_load_dls(stream->midi_data, stream->midi_size,
                                     stream->sample_rate, MID_AUDIO_S16LSB, 2,
                                     stream->sample_rate * 2 * 2);
    
    if (stream->song)
    {
        mid_song_start(stream->song);
    }
    
    // Note: If reload fails, song will be nullptr and subsequent ReadMIDI calls will return 0
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
const u16char plugin_intro[] = U16_TEXT("MIDI 音频文件解码(Libtimidity)");

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
    if (timidity_initialized)
    {
        mid_exit();
        timidity_initialized = false;
    }
}
//--------------------------------------------------------------------------------------------------
