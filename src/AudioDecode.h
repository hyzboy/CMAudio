#ifndef HGL_AUDIO_DECODE_INCLUDE
#define HGL_AUDIO_DECODE_INCLUDE

#include<hgl/audio/OpenAL.h>
#include<hgl/type/String.h>

using namespace openal;
namespace hgl
{
    struct AudioPlugInInterface
    {
        void    (AL_APIENTRY *Load      )(ALbyte *,ALsizei,ALenum *,ALvoid **,ALsizei *,ALsizei *,ALboolean *);
        void    (AL_APIENTRY *Clear     )(ALenum,ALvoid *,ALsizei,ALsizei);

        void *  (AL_APIENTRY *Open      )(ALbyte *,ALsizei,ALenum *,ALsizei *,double *);
        void    (AL_APIENTRY *Close     )(void *);
        uint    (AL_APIENTRY *Read      )(void *,char *,uint);
        void    (AL_APIENTRY *Restart   )(void *);
    };//struct AudioPlugInInterface

    struct AudioFloatPlugInInterface
    {
        void    (AL_APIENTRY *Load      )(ALbyte *,ALsizei,ALenum *,float **,ALsizei *,ALsizei *,ALboolean *);
        void    (AL_APIENTRY *Clear     )(ALenum,ALvoid *,ALsizei,ALsizei);

        uint    (AL_APIENTRY *Read      )(void *,float *,uint);
    };//struct AudioPlugInInterface

    struct AudioMidiConfigInterface
    {
        void    (AL_APIENTRY *SetSoundFont      )(const char *);      // Set soundfont/bank file path for MIDI synthesis
        void    (AL_APIENTRY *SetBankID         )(int);               // Set bank ID for chip emulators (OPNMIDI/ADLMIDI)
        void    (AL_APIENTRY *SetVolume         )(float);             // Set global volume (0.0-1.0)
        void    (AL_APIENTRY *SetSampleRate     )(int);               // Set sample rate (default: 44100)
        void    (AL_APIENTRY *SetPolyphony      )(int);               // Set max polyphony/voices (FluidSynth only)
        void    (AL_APIENTRY *SetChipCount      )(int);               // Set number of emulated chips (OPNMIDI/ADLMIDI)
        void    (AL_APIENTRY *EnableReverb      )(bool);              // Enable/disable reverb (FluidSynth only)
        void    (AL_APIENTRY *EnableChorus      )(bool);              // Enable/disable chorus (FluidSynth only)
        const char* (AL_APIENTRY *GetVersionString)();                // Get plugin version string
        const char* (AL_APIENTRY *GetDefaultBank)();                  // Get default bank/soundfont path
    };//struct AudioMidiConfigInterface

    struct MidiChannelInfo
    {
        int channel;                    // Channel number (0-15)
        int program;                    // Current program/instrument (0-127)
        int bank;                       // Bank number (0-16383)
        float volume;                   // Channel volume (0.0-1.0)
        float pan;                      // Pan position (-1.0=left, 0.0=center, 1.0=right)
        bool muted;                     // Is channel muted
        bool solo;                      // Is channel soloed
        int note_count;                 // Number of active notes on this channel
        const char* instrument_name;    // Name of current instrument (if available)
    };//struct MidiChannelInfo

    struct AudioMidiChannelInterface
    {
        // Channel information
        int     (AL_APIENTRY *GetChannelCount   )();                          // Get total number of MIDI channels (usually 16)
        bool    (AL_APIENTRY *GetChannelInfo    )(int channel, MidiChannelInfo*);  // Get information about a specific channel
        
        // Channel control
        void    (AL_APIENTRY *SetChannelProgram )(int channel, int program);  // Change instrument on a channel (0-127)
        void    (AL_APIENTRY *SetChannelBank    )(int channel, int bank);     // Change bank for a channel
        void    (AL_APIENTRY *SetChannelVolume  )(int channel, float volume); // Set channel volume (0.0-1.0)
        void    (AL_APIENTRY *SetChannelPan     )(int channel, float pan);    // Set channel pan (-1.0 to 1.0)
        void    (AL_APIENTRY *MuteChannel       )(int channel, bool mute);    // Mute/unmute a channel
        void    (AL_APIENTRY *SoloChannel       )(int channel, bool solo);    // Solo/unsolo a channel
        
        // Multi-channel decoding
        void*   (AL_APIENTRY *OpenMultiChannel  )(ALbyte *data, ALsizei size, ALenum *format, ALsizei *freq, double *time);  // Open for multi-channel decode
        uint    (AL_APIENTRY *ReadChannel       )(void *stream, int channel, char *pcm_data, uint buf_size);  // Read specific channel
        uint    (AL_APIENTRY *ReadChannels      )(void *stream, int *channels, int channel_count, char **pcm_data, uint buf_size);  // Read multiple channels separately
        void    (AL_APIENTRY *CloseMultiChannel )(void *stream);              // Close multi-channel stream
    };//struct AudioMidiChannelInterface

    bool GetAudioInterface(const OSString &,AudioPlugInInterface *,AudioFloatPlugInInterface *);
    bool GetAudioMidiInterface(const OSString &,AudioMidiConfigInterface *);
    bool GetAudioMidiChannelInterface(const OSString &,AudioMidiChannelInterface *);
}//namespace hgl
#endif//HGL_AUDIO_DECODE_INCLUDE
