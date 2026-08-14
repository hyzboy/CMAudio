#include<hgl/audio/AudioCapture.h>

namespace hgl::audio
{
    AudioCapture::AudioCapture()
    {
        device=nullptr;
        sample_rate=44100;
        sample_format=AL_FORMAT_MONO16;
        buffer_size=0;
    }

    AudioCapture::~AudioCapture()
    {
        Close();
    }

    bool AudioCapture::Open(uint sr,uint format,double buffer_seconds)
    {
        Close();

        if(!openal::alcCaptureOpenDevice)
            return false;

        const openal::ALCsizei buf_size=(openal::ALCsizei)(sr*buffer_seconds);

        device=openal::alcCaptureOpenDevice(nullptr,sr,format,buf_size);

        if(!device)
            return false;

        sample_rate=sr;
        sample_format=format;
        buffer_size=buf_size;

        return true;
    }

    void AudioCapture::Close()
    {
        if(device&&openal::alcCaptureCloseDevice)
        {
            openal::alcCaptureCloseDevice(device);
            device=nullptr;
        }

        buffer_size=0;
    }

    bool AudioCapture::Start()
    {
        if(!device||!openal::alcCaptureStart)
            return false;

        openal::alcCaptureStart(device);
        return true;
    }

    bool AudioCapture::Stop()
    {
        if(!device||!openal::alcCaptureStop)
            return false;

        openal::alcCaptureStop(device);
        return true;
    }

    int AudioCapture::GetAvailableSamples()const
    {
        if(!device||!openal::alcGetIntegerv)
            return 0;

        openal::ALCint samples=0;

        openal::alcGetIntegerv(device,ALC_CAPTURE_SAMPLES,sizeof(openal::ALCint),&samples);

        return (int)samples;
    }

    int AudioCapture::ReadSamples(void *buffer,int sample_count)
    {
        if(!device||!openal::alcCaptureSamples||!buffer||sample_count<=0)
            return 0;

        const int available=GetAvailableSamples();

        if(available<=0)
            return 0;

        const int to_read=(sample_count<available)?sample_count:available;

        openal::alcCaptureSamples(device,buffer,to_read);

        return to_read;
    }
}//namespace hgl::audio
