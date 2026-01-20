#define __MAKE_PLUGIN__

#include<hgl/plugin/PlugIn.h>
#include<hgl/plugin/PlugInInterface.h>
#include<malloc.h>
#include<string.h>
#include<hgl/al/al.h>

#include<opnmidi.h>

using namespace hgl;
using namespace openal;

// OPNMIDI global state
static OPN2_MIDIPlayer* g_opn_device = nullptr;
static bool opnmidi_initialized = false;

static bool InitOPNMIDI()
{
    if (opnmidi_initialized && g_opn_device)
        return true;
        
    // Create OPN2 (YM2612) device with 44.1kHz sample rate
    g_opn_device = opn2_init(44100);
    if (!g_opn_device)
        return false;
    
    // Set 4-chip mode for higher polyphony (Sega Genesis used 1 chip)
    opn2_setNumChips(g_opn_device, 4);
    
    // Use built-in WOPN bank (Yamaha OPN2 patches)
    if (opn2_openBankData(g_opn_device, nullptr, 0) < 0)
    {
        opn2_close(g_opn_device);
        g_opn_device = nullptr;
        return false;
    }
    
    opnmidi_initialized = true;
    return true;
}

ALvoid LoadMIDI(ALbyte *memory, ALsizei memory_size, ALenum *format, ALvoid **data, ALsizei *size, ALsizei *freq, ALboolean *loop)
{
    if (!InitOPNMIDI())
        return;

    // Create temporary device for this load operation
    OPN2_MIDIPlayer* device = opn2_init(44100);
    if (!device)
        return;
    
    opn2_setNumChips(device, 4);
    opn2_openBankData(device, nullptr, 0);
    
    // Load MIDI from memory
    if (opn2_openData(device, memory, memory_size) < 0)
    {
        opn2_close(device);
        return;
    }

    // OPNMIDI outputs stereo 16-bit
    *format = AL_FORMAT_STEREO16;
    *freq = 44100;

    // Get total time
    double total_time = opn2_totalTimeLength(device);
    
    size_t total_samples = (size_t)(total_time * 44100);
    const size_t total_stereo_samples = total_samples * 2; // stereo
    const size_t pcm_total_bytes = total_stereo_samples * sizeof(int16_t);

    int16_t *ptr = new int16_t[total_stereo_samples];
    size_t out_size = 0;

    // Render MIDI to PCM
    const size_t render_samples = 4096; // samples per channel
    
    while (out_size < pcm_total_bytes)
    {
        size_t samples_to_render = render_samples;
        if (out_size + samples_to_render * 2 * sizeof(int16_t) > pcm_total_bytes)
            samples_to_render = (pcm_total_bytes - out_size) / (2 * sizeof(int16_t));
        
        if (samples_to_render == 0)
            break;
            
        int rendered = opn2_playFormat(device, (int)samples_to_render, 
                                       (OPN2_UInt8*)((char*)ptr + out_size),
                                       (OPN2_UInt8*)((char*)ptr + out_size) + sizeof(int16_t),
                                       nullptr);
        
        if (rendered <= 0)
            break;
            
        out_size += rendered * 2 * sizeof(int16_t);
    }

    *size = (int)out_size;
    *data = ptr;

    opn2_close(device);
}

void ClearMIDI(ALenum, ALvoid *data, ALsizei, ALsizei)
{
    if (data)
        delete[] (int16_t *)data;
}

//--------------------------------------------------------------------------------------------------
struct MidiStream
{
    OPN2_MIDIPlayer* device;
    unsigned char *midi_data;
    size_t midi_size;
    unsigned long sample_rate;
};

void *OpenMIDI(ALbyte *memory, ALsizei memory_size, ALenum *format, ALsizei *rate, double *total_time)
{
    if (!InitOPNMIDI())
        return nullptr;

    MidiStream *stream = new MidiStream;
    
    // Store MIDI data for potential restart
    stream->midi_data = (unsigned char*)memory;
    stream->midi_size = memory_size;
    stream->sample_rate = 44100;
    
    stream->device = opn2_init(stream->sample_rate);
    if (!stream->device)
    {
        delete stream;
        return nullptr;
    }
    
    opn2_setNumChips(stream->device, 4);
    opn2_openBankData(stream->device, nullptr, 0);
    
    // Load MIDI from memory
    if (opn2_openData(stream->device, stream->midi_data, stream->midi_size) < 0)
    {
        opn2_close(stream->device);
        delete stream;
        return nullptr;
    }

    // OPNMIDI outputs stereo 16-bit
    *format = AL_FORMAT_STEREO16;
    *rate = stream->sample_rate;

    // Get total time
    *total_time = opn2_totalTimeLength(stream->device);

    return stream;
}

void CloseMIDI(void *ptr)
{
    MidiStream *stream = (MidiStream *)ptr;
    
    if (stream->device)
        opn2_close(stream->device);
    
    delete stream;
}

uint ReadMIDI(void *ptr, char *data, uint buf_max)
{
    MidiStream *stream = (MidiStream *)ptr;
    
    if (!stream || !stream->device)
        return 0;
    
    // Calculate samples to render
    size_t samples = buf_max / (2 * sizeof(int16_t)); // stereo 16-bit
    
    int rendered = opn2_playFormat(stream->device, (int)samples,
                                   (OPN2_UInt8*)data,
                                   (OPN2_UInt8*)data + sizeof(int16_t),
                                   nullptr);
    
    if (rendered <= 0)
        return 0;

    return rendered * 2 * sizeof(int16_t);
}

void RestartMIDI(void *ptr)
{
    MidiStream *stream = (MidiStream *)ptr;
    
    if (!stream || !stream->device)
        return;
    
    // Reset position to beginning
    opn2_positionRewind(stream->device);
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
const u16char plugin_intro[] = U16_TEXT("MIDI 音频文件解码(OPNMIDI - YM2612 芯片模拟)");

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
    if (opnmidi_initialized && g_opn_device)
    {
        opn2_close(g_opn_device);
        g_opn_device = nullptr;
        opnmidi_initialized = false;
    }
}
//--------------------------------------------------------------------------------------------------
