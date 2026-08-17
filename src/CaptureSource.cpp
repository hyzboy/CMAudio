#include<hgl/audio/CaptureSource.h>
#include<hgl/time/Time.h>
#include<cstdint>
#include<cmath>

namespace hgl::audio
{
    namespace
    {
        constexpr float CS_PI       = 3.14159265358979323846f;
        constexpr float CS_MOCK_FREQ = 1000.0f;     ///< mock 信号频率（1kHz）
        constexpr float CS_MOCK_AMP  = 0.25f;       ///< mock 信号幅度
    }//namespace

    CaptureSource::CaptureSource()
    {
        device=nullptr;
        sample_rate=16000;
        sample_format=AL_FORMAT_MONO16;
        frame_samples=0;
        buffer_size=0;
        mock=false;
        mock_phase=0;
    }

    CaptureSource::~CaptureSource()
    {
        Close();
    }

    bool CaptureSource::Open(uint sr,uint frame_ms,uint format,bool use_mock)
    {
        Close();

        if(sr==0)
            sr=16000;

        if(frame_ms==0)
            frame_ms=20;

        sample_rate=sr;
        sample_format=format;
        frame_samples=(uint)((double)sr*(double)frame_ms/1000.0);

        if(frame_samples==0)
            frame_samples=1;

        if(use_mock)
        {
            mock=true;
            mock_phase=0;
            buffer_size=frame_samples*4;
            return true;
        }

        mock=false;

        if(!openal::alcCaptureOpenDevice)
            return false;

        const openal::ALCsizei buf_size=(openal::ALCsizei)(frame_samples*4);      // 4 帧设备缓冲

        device=openal::alcCaptureOpenDevice(nullptr,sr,format,buf_size);

        if(!device)
            return false;

        buffer_size=buf_size;

        return true;
    }

    void CaptureSource::Close()
    {
        if(device&&openal::alcCaptureCloseDevice)
        {
            openal::alcCaptureCloseDevice(device);
            device=nullptr;
        }

        mock=false;
        buffer_size=0;
    }

    bool CaptureSource::Start()
    {
        if(mock)
            return true;

        if(!device||!openal::alcCaptureStart)
            return false;

        openal::alcCaptureStart(device);
        return true;
    }

    bool CaptureSource::Stop()
    {
        if(mock)
            return true;

        if(!device||!openal::alcCaptureStop)
            return false;

        openal::alcCaptureStop(device);
        return true;
    }

    int CaptureSource::GetAvailableSamples()const
    {
        if(mock)
            return (int)frame_samples;

        if(!device||!openal::alcGetIntegerv)
            return 0;

        openal::ALCint samples=0;

        openal::alcGetIntegerv(device,ALC_CAPTURE_SAMPLES,sizeof(openal::ALCint),&samples);

        return (int)samples;
    }

    int CaptureSource::ReadFrame(void *buffer,int max_samples)
    {
        if(!buffer||max_samples<=0)
            return 0;

        const int want=(frame_samples<(uint)max_samples)?(int)frame_samples:max_samples;

        if(mock)
        {
            int16_t *dst=(int16_t*)buffer;

            for(int i=0;i<want;i++)
            {
                const float t=(float)(mock_phase+i)/(float)sample_rate;
                const float s=CS_MOCK_AMP*std::sin(2.0f*CS_PI*CS_MOCK_FREQ*t);

                dst[i]=(int16_t)(s*32767.0f);
            }

            mock_phase+=want;

            return want*2;                      // 16bit mono 字节数
        }

        if(!device||!openal::alcCaptureSamples)
            return 0;

        // 轮询等待至一帧完整（最多等一帧时长；5ms 粒度）
        const int max_waits=(int)(frame_samples*1000/sample_rate/5)+1;

        int avail=0;

        for(int i=0;i<max_waits;i++)
        {
            openal::ALCint s=0;

            openal::alcGetIntegerv(device,ALC_CAPTURE_SAMPLES,sizeof(openal::ALCint),&s);

            avail=(int)s;

            if(avail>=want)
                break;

            SleepSecond(0.005);
        }

        if(avail<=0)
            return 0;

        const int to_read=(avail<want)?avail:want;

        openal::alcCaptureSamples(device,buffer,to_read);

        return to_read*2;                       // 16bit mono 字节数
    }
}//namespace hgl::audio
