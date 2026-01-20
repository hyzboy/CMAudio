#define __MAKE_PLUGIN__

#include<hgl/plugin/PlugIn.h>
#include<hgl/plugin/PlugInInterface.h>
#include<malloc.h>
#include<string.h>
#include<hgl/al/al.h>

#include<adlmidi.h>

using namespace hgl;
using namespace openal;

// ADLMIDI global state
static ADL_MIDIPlayer* g_adl_device = nullptr;
static bool adlmidi_initialized = false;

// Configuration state
static char custom_bank_path[512] = {0};
static int bank_id = 58;  // Default: DMX OP2 bank
static float global_volume = 1.0f;
static int sample_rate = 44100;
static int chip_count = 4;

static const char* GetBankPath()
{
    // Priority 1: Custom path set via API
    if (custom_bank_path[0] != '\0')
        return custom_bank_path;
    
    // Priority 2: Use built-in banks (nullptr)
    return nullptr;
}

static bool InitADLMIDI()
{
    if (adlmidi_initialized && g_adl_device)
        return true;
        
    // Create AdLib/OPL3 device using configuration
    g_adl_device = adl_init(sample_rate);
    if (!g_adl_device)
        return false;
    
    // Set chip count for polyphony
    adl_setNumChips(g_adl_device, chip_count);
    
    // Set volume
    adl_setVolumeRangeModel(g_adl_device, ADLMIDI_VolumeModel_AUTO);
    
    // Load bank
    const char* bank_path = GetBankPath();
    if (bank_path != nullptr)
    {
        // Load from file if custom path is set
        if (adl_openBankFile(g_adl_device, bank_path) < 0)
        {
            adl_close(g_adl_device);
            g_adl_device = nullptr;
            return false;
        }
    }
    else
    {
        // Use built-in bank ID
        if (adl_setBank(g_adl_device, bank_id) < 0)
        {
            // Try default bank (0) if selected bank fails
            adl_setBank(g_adl_device, 0);
        }
    }
    
    adlmidi_initialized = true;
    return true;
}

ALvoid LoadMIDI(ALbyte *memory, ALsizei memory_size, ALenum *format, ALvoid **data, ALsizei *size, ALsizei *freq, ALboolean *loop)
{
    if (!InitADLMIDI())
        return;

    // Create temporary device for this load operation
    ADL_MIDIPlayer* device = adl_init(sample_rate);
    if (!device)
        return;
    
    adl_setNumChips(device, chip_count);
    const char* bank_path = GetBankPath();
    if (bank_path)
        adl_openBankFile(device, bank_path);
    else
        adl_setBank(device, bank_id);
    
    // Load MIDI from memory
    if (adl_openData(device, memory, memory_size) < 0)
    {
        adl_close(device);
        return;
    }

    // ADLMIDI outputs stereo 16-bit
    *format = AL_FORMAT_STEREO16;
    *freq = sample_rate;

    // Get total time
    double total_time = adl_totalTimeLength(device);
    
    size_t total_samples = (size_t)(total_time * sample_rate);
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
            
        int rendered = adl_playFormat(device, (int)samples_to_render, 
                                      (ADL_UInt8*)((char*)ptr + out_size),
                                      (ADL_UInt8*)((char*)ptr + out_size) + sizeof(int16_t),
                                      nullptr);
        
        if (rendered <= 0)
            break;
            
        out_size += rendered * 2 * sizeof(int16_t);
    }

    *size = (int)out_size;
    *data = ptr;

    adl_close(device);
}

void ClearMIDI(ALenum, ALvoid *data, ALsizei, ALsizei)
{
    if (data)
        delete[] (int16_t *)data;
}

//--------------------------------------------------------------------------------------------------
struct MidiStream
{
    ADL_MIDIPlayer* device;
    unsigned char *midi_data;
    size_t midi_size;
    unsigned long sample_rate;
};

void *OpenMIDI(ALbyte *memory, ALsizei memory_size, ALenum *format, ALsizei *rate, double *total_time)
{
    if (!InitADLMIDI())
        return nullptr;

    MidiStream *stream = new MidiStream;
    
    // Store MIDI data for potential restart
    stream->midi_data = (unsigned char*)memory;
    stream->midi_size = memory_size;
    stream->sample_rate = 44100;
    
    stream->device = adl_init(stream->sample_rate);
    if (!stream->device)
    {
        delete stream;
        return nullptr;
    }
    
    adl_setNumChips(stream->device, chip_count);
    const char* bank_path = GetBankPath();
    if (bank_path)
        adl_openBankFile(stream->device, bank_path);
    else
        adl_setBank(stream->device, bank_id);
    
    // Load MIDI from memory
    if (adl_openData(stream->device, stream->midi_data, stream->midi_size) < 0)
    {
        adl_close(stream->device);
        delete stream;
        return nullptr;
    }

    // ADLMIDI outputs stereo 16-bit
    *format = AL_FORMAT_STEREO16;
    *rate = stream->sample_rate;

    // Get total time
    *total_time = adl_totalTimeLength(stream->device);

    return stream;
}

void CloseMIDI(void *ptr)
{
    MidiStream *stream = (MidiStream *)ptr;
    
    if (stream->device)
        adl_close(stream->device);
    
    delete stream;
}

uint ReadMIDI(void *ptr, char *data, uint buf_max)
{
    MidiStream *stream = (MidiStream *)ptr;
    
    if (!stream || !stream->device)
        return 0;
    
    // Calculate samples to render
    size_t samples = buf_max / (2 * sizeof(int16_t)); // stereo 16-bit
    
    int rendered = adl_playFormat(stream->device, (int)samples,
                                  (ADL_UInt8*)data,
                                  (ADL_UInt8*)data + sizeof(int16_t),
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
    adl_positionRewind(stream->device);
}

//--------------------------------------------------------------------------------------------------
// MIDI Configuration Interface
//--------------------------------------------------------------------------------------------------

void SetSoundFont(const char* path)
{
    // ADLMIDI uses bank files (.wopl), not SoundFonts
    // This maps to SetBankPath for consistency
    if (path && path[0] != '\0')
    {
        strncpy(custom_bank_path, path, sizeof(custom_bank_path) - 1);
        custom_bank_path[sizeof(custom_bank_path) - 1] = '\0';
        
        // If already initialized, reload the bank
        if (adlmidi_initialized && g_adl_device)
        {
            adl_openBankFile(g_adl_device, path);
        }
    }
}

void SetBankID(int id)
{
    bank_id = id;
    if (adlmidi_initialized && g_adl_device)
    {
        adl_setBank(g_adl_device, id);
    }
}

void SetVolume(float volume)
{
    global_volume = volume;
    if (adlmidi_initialized && g_adl_device)
    {
        adl_setVolume(g_adl_device, volume);
    }
}

void SetSampleRate(int rate)
{
    sample_rate = rate;
    // Requires reinitialization to take effect
}

void SetPolyphony(int poly)
{
    // ADLMIDI polyphony is controlled by chip count
    // Each chip provides 18 channels, so chip_count = poly / 18
    chip_count = (poly + 17) / 18;  // Round up
    if (chip_count < 1) chip_count = 1;
    if (chip_count > 100) chip_count = 100;
    
    if (adlmidi_initialized && g_adl_device)
    {
        adl_setNumChips(g_adl_device, chip_count);
    }
}

void SetChipCount(int count)
{
    chip_count = count;
    if (chip_count < 1) chip_count = 1;
    if (chip_count > 100) chip_count = 100;
    
    if (adlmidi_initialized && g_adl_device)
    {
        adl_setNumChips(g_adl_device, chip_count);
    }
}

void EnableReverb(bool enable)
{
    // ADLMIDI doesn't have built-in reverb
    // This is a no-op for ADLMIDI
}

void EnableChorus(bool enable)
{
    // ADLMIDI doesn't have built-in chorus
    // This is a no-op for ADLMIDI
}

const char* GetVersion()
{
    return "ADLMIDI - OPL3 Chip Emulator";
}

const char* GetDefaultBank()
{
    const char* path = GetBankPath();
    if (path)
        return path;
    
    // Return built-in bank description
    // Note: This returns a constant string, which is thread-safe
    switch (bank_id)
    {
        case 0: return "Built-in Bank 0 (Generic)";
        case 58: return "Built-in Bank 58 (DMX OP2)";
        default: return "Built-in Bank (Custom ID)";
    }
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

//--------------------------------------------------------------------------------------------------
const u16char plugin_intro[] = U16_TEXT("MIDI 音频文件解码(ADLMIDI - OPL3 芯片模拟)");

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
    if (adlmidi_initialized && g_adl_device)
    {
        adl_close(g_adl_device);
        g_adl_device = nullptr;
        adlmidi_initialized = false;
    }
}
//--------------------------------------------------------------------------------------------------
