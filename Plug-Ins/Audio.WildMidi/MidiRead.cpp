#define __MAKE_PLUGIN__

#include<hgl/plugin/PlugIn.h>
#include<hgl/plugin/PlugInInterface.h>
#include<malloc.h>
#include<string.h>
#include<hgl/al/al.h>

#include<wildmidi_lib.h>

using namespace hgl;
using namespace openal;

// WildMIDI global initialization state
static bool wildmidi_initialized = false;

// Get config path from environment or use default
static const char* GetConfigPath()
{
    const char* config_path = getenv("WILDMIDI_CFG");
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
    if (!wildmidi_initialized)
    {
        const char* config_path = GetConfigPath();
        if (WildMidi_Init(config_path, 44100, 0) == -1)
        {
            return;  // Failed to initialize
        }
        wildmidi_initialized = true;
    }

    midi *handle = WildMidi_OpenBuffer((unsigned char*)memory, memory_size);
    if (!handle)
        return;

    struct _WM_Info *info = WildMidi_GetInfo(handle);
    if (!info)
    {
        WildMidi_Close(handle);
        return;
    }

    // WildMIDI always outputs stereo 16-bit
    *format = AL_FORMAT_STEREO16;
    *freq = 44100;

    // Calculate approximate total size based on MIDI duration
    unsigned long approx_samples = info->approx_total_samples;
    const size_t total_samples = approx_samples * 2; // stereo
    const size_t pcm_total_bytes = total_samples * sizeof(int16_t);

    int16_t *ptr = new int16_t[total_samples];
    int out_size = 0;
    int read_size;

    // Read all MIDI data converted to PCM
    while (out_size < (int)pcm_total_bytes)
    {
        read_size = WildMidi_GetOutput(handle, (int8_t*)((char*)ptr + out_size), pcm_total_bytes - out_size);
        
        if (read_size <= 0)
            break;
            
        out_size += read_size;
    }

    *size = out_size;
    *data = ptr;

    WildMidi_Close(handle);
}

void ClearMIDI(ALenum, ALvoid *data, ALsizei, ALsizei)
{
    if (data)
        delete[] (int16_t *)data;
}

//--------------------------------------------------------------------------------------------------
struct MidiStream
{
    midi *handle;
    unsigned char *midi_data;
    size_t midi_size;
    unsigned long sample_rate;
};

void *OpenMIDI(ALbyte *memory, ALsizei memory_size, ALenum *format, ALsizei *rate, double *total_time)
{
    if (!wildmidi_initialized)
    {
        const char* config_path = GetConfigPath();
        if (WildMidi_Init(config_path, 44100, 0) == -1)
        {
            return nullptr;  // Failed to initialize
        }
        wildmidi_initialized = true;
    }

    MidiStream *stream = new MidiStream;
    
    // Store MIDI data for potential restart
    stream->midi_data = (unsigned char*)memory;
    stream->midi_size = memory_size;
    
    stream->handle = WildMidi_OpenBuffer(stream->midi_data, stream->midi_size);
    if (!stream->handle)
    {
        delete stream;
        return nullptr;
    }

    struct _WM_Info *info = WildMidi_GetInfo(stream->handle);
    if (!info)
    {
        WildMidi_Close(stream->handle);
        delete stream;
        return nullptr;
    }

    // WildMIDI always outputs stereo 16-bit
    *format = AL_FORMAT_STEREO16;
    stream->sample_rate = 44100;
    *rate = stream->sample_rate;

    // Calculate total time from approximate total samples
    *total_time = (double)info->approx_total_samples / (double)stream->sample_rate;

    return stream;
}

void CloseMIDI(void *ptr)
{
    MidiStream *stream = (MidiStream *)ptr;
    
    if (stream->handle)
        WildMidi_Close(stream->handle);
    
    delete stream;
}

uint ReadMIDI(void *ptr, char *data, uint buf_max)
{
    MidiStream *stream = (MidiStream *)ptr;
    
    if (!stream || !stream->handle)
        return 0;
    
    int result = WildMidi_GetOutput(stream->handle, (int8_t*)data, buf_max);

    if (result <= 0)
        return 0;

    return result;
}

void RestartMIDI(void *ptr)
{
    MidiStream *stream = (MidiStream *)ptr;
    
    if (!stream)
        return;
    
    // Close current handle
    if (stream->handle)
    {
        WildMidi_Close(stream->handle);
        stream->handle = nullptr;
    }
    
    // Reopen to ensure clean restart
    stream->handle = WildMidi_OpenBuffer(stream->midi_data, stream->midi_size);
    
    // Note: If reopen fails, handle will be nullptr and subsequent ReadMIDI calls will return 0
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
const u16char plugin_intro[] = U16_TEXT("MIDI 音频文件解码(WildMIDI 0.4.x)");

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
    if (wildmidi_initialized)
    {
        WildMidi_Shutdown();
        wildmidi_initialized = false;
    }
}
//--------------------------------------------------------------------------------------------------
