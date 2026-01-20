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
        const char* (AL_APIENTRY *GetVersion    )();                  // Get plugin version string
        const char* (AL_APIENTRY *GetDefaultBank)();                  // Get default bank/soundfont path
    };//struct AudioMidiConfigInterface

    bool GetAudioInterface(const OSString &,AudioPlugInInterface *,AudioFloatPlugInInterface *);
    bool GetAudioMidiInterface(const OSString &,AudioMidiConfigInterface *);
}//namespace hgl
#endif//HGL_AUDIO_DECODE_INCLUDE
