#include<hgl/audio/VoicePreprocess.h>
#include<hgl/audio/AudioAnalysis.h>
#include<cmath>
#include<cstring>

namespace hgl::audio
{
    namespace
    {
        const float PI = 3.14159265358979323846f;

        // 原地复数 IFFT：IFFT(X) = conj(FFT(conj(X)))/n
        void IFFT(float *real,float *imag,int n)
        {
            for(int i=0;i<n;i++)
                imag[i]=-imag[i];

            FFT(real,imag,n);

            for(int i=0;i<n;i++)
            {
                real[i]/=n;
                imag[i]=-imag[i]/n;
            }
        }

        float FrameRMSDB(const float *in,int count)
        {
            float sum=0;
            for(int i=0;i<count;i++)
                sum+=in[i]*in[i];

            if(sum<=0)return(-120.0f);

            return(10.0f*std::log10(sum/count));
        }
    }//namespace

    //====================================================================
    // NoiseSuppressor
    //====================================================================
    NoiseSuppressor::NoiseSuppressor()
    {
        sample_rate=0;
        frame_samples=0;
        nfft=0;
        bin_count=0;
        noise_frames=0;
        noise_floor_db=-120.0f;
        noise_alpha=0.1f;
        speech_threshold_db=10.0f;
        sub_alpha=2.0f;
        sub_floor=0.02f;
    }

    void NoiseSuppressor::Init(uint sr,uint fs)
    {
        sample_rate=sr;
        frame_samples=fs;

        nfft=1;
        while(nfft<frame_samples)
            nfft<<=1;
        if(nfft<128)nfft=128;       // 保证频域分辨率

        bin_count=nfft;     // 含负频率：谱减必须覆盖全部频点，否则负频率残留一半噪声能量

        window.resize(nfft);
        for(uint i=0;i<nfft;i++)
            window[i]=0.5f-0.5f*std::cos(2.0f*PI*(float)i/(float)(nfft-1));

        real.resize(nfft);
        imag.resize(nfft);
        noise_floor.assign(bin_count,0.0f);
        buf.assign(nfft,0.0f);

        Reset();
    }

    void NoiseSuppressor::Reset()
    {
        noise_frames=0;
        noise_floor_db=-120.0f;
        std::fill(noise_floor.begin(),noise_floor.end(),0.0f);
        std::fill(buf.begin(),buf.end(),0.0f);
    }

    void NoiseSuppressor::Process(const float *in,float *out)
    {
        // 1. 输入入缓冲（矩形窗：不做幅度调制，保证帧间时域连续）
        for(uint i=0;i<frame_samples;i++)
            buf[i]=in[i];
        for(uint i=frame_samples;i<nfft;i++)
            buf[i]=0;

        // 2. FFT
        for(uint i=0;i<nfft;i++)
        {
            real[i]=buf[i];
            imag[i]=0;
        }
        FFT(real.data(),imag.data(),(int)nfft);

        // 3. 帧能量判定 + 噪声底更新
        const float frame_db=FrameRMSDB(in,(int)frame_samples);

        std::vector<float> mag(bin_count);
        for(uint k=0;k<bin_count;k++)
            mag[k]=std::sqrt(real[k]*real[k]+imag[k]*imag[k]);

        // 预热期（前 10 帧）无条件更新噪声底；之后仅当帧能量低于"噪声底+阈值"（静音帧）时更新
        if(noise_frames<10||frame_db<noise_floor_db+speech_threshold_db)
        {
            for(uint k=0;k<bin_count;k++)
            {
                if(noise_floor[k]<=0)
                    noise_floor[k]=mag[k];
                else
                    noise_floor[k]=(1.0f-noise_alpha)*noise_floor[k]+noise_alpha*mag[k];
            }

            if(noise_floor_db<=-110.0f)
                noise_floor_db=frame_db;
            else
                noise_floor_db=(1.0f-noise_alpha)*noise_floor_db+noise_alpha*frame_db;

            noise_frames++;
        }

        // 4. 功率域谱减：new_power = max(power - α·noise_power, β·power)
        for(uint k=0;k<bin_count;k++)
        {
            const float power=real[k]*real[k]+imag[k]*imag[k];
            const float np=noise_floor[k]*noise_floor[k];

            float new_power=power-sub_alpha*np;
            if(new_power<sub_floor*power)
                new_power=sub_floor*power;
            if(new_power<0)new_power=0;

            const float scale=(power>1e-12f)?(std::sqrt(new_power/power)):0.0f;

            real[k]*=scale;
            imag[k]*=scale;
        }

        // 5. IFFT
        IFFT(real.data(),imag.data(),(int)nfft);

        // 6. 输出前 frame_samples 样本（矩形窗无幅度调制，直接输出）
        memcpy(out,real.data(),frame_samples*sizeof(float));
    }

    //====================================================================
    // AutoGainControl
    //====================================================================
    AutoGainControl::AutoGainControl()
    {
        target_rms=0.1f;
        attack=0.2f;
        release=0.05f;
        cur_gain=1.0f;
    }

    void AutoGainControl::Init(float target_rms_linear)
    {
        target_rms=target_rms_linear;
        attack=0.2f;
        release=0.05f;
        cur_gain=1.0f;
    }

    void AutoGainControl::Reset()
    {
        cur_gain=1.0f;
    }

    void AutoGainControl::Process(const float *in,float *out,uint frame_samples)
    {
        const float rms=ComputeRMS(in,(int)frame_samples);

        float desired=1.0f;

        if(rms>1e-6f)
            desired=target_rms/rms;

        // 平滑：增益上升（信号变弱）用 attack，下降（信号变强）用 release
        const float alpha=(desired>cur_gain)?attack:release;

        cur_gain+=(desired-cur_gain)*alpha;

        for(uint i=0;i<frame_samples;i++)
        {
            float v=in[i]*cur_gain;

            // 软限幅（tanh 特性）防削波
            if(v>1.0f)v=1.0f;else
            if(v<-1.0f)v=-1.0f;

            out[i]=v;
        }
    }

    //====================================================================
    // VoiceActivityDetector
    //====================================================================
    VoiceActivityDetector::VoiceActivityDetector()
    {
        noise_floor_db=-50.0f;
        threshold_db=15.0f;
        hangover=5;
        hang_count=0;
        noise_alpha=0.05f;
    }

    void VoiceActivityDetector::Init(float thr_db,int hangover_frames)
    {
        threshold_db=thr_db;
        hangover=hangover_frames;
        noise_alpha=0.05f;
        noise_floor_db=-50.0f;
        hang_count=0;
    }

    void VoiceActivityDetector::Reset()
    {
        noise_floor_db=-50.0f;
        hang_count=0;
    }

    bool VoiceActivityDetector::Process(const float *in,uint frame_samples)
    {
        const float frame_db=FrameRMSDB(in,(int)frame_samples);

        // 更新噪声底（只在低电平帧更新，且慢速）
        if(frame_db<noise_floor_db+threshold_db)
            noise_floor_db+=(frame_db-noise_floor_db)*noise_alpha;

        const bool is_speech=(frame_db>noise_floor_db+threshold_db);

        if(is_speech)
        {
            hang_count=hangover;
            return(true);
        }

        if(hang_count>0)
        {
            hang_count--;
            return(true);       // hangover 期间仍算语音
        }

        return(false);
    }

    //====================================================================
    // VoicePreprocess
    //====================================================================
    void VoicePreprocess::Init(uint sample_rate,uint frame_samples)
    {
        ns.Init(sample_rate,frame_samples);
        agc.Init(0.1f);
        vad.Init(15.0f,5);

        ns_out.resize(frame_samples);
    }

    void VoicePreprocess::Reset()
    {
        ns.Reset();
        agc.Reset();
        vad.Reset();
    }

    bool VoicePreprocess::Process(const float *in,float *out,uint frame_samples)
    {
        ns.Process(in,ns_out.data());

        agc.Process(ns_out.data(),out,frame_samples);

        return(vad.Process(out,frame_samples));
    }
}//namespace hgl::audio
