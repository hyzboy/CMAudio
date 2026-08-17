#include<hgl/audio/VoiceCall.h>
#include<hgl/audio/AudioAnalysis.h>
#include<random>
#include<cstdint>

namespace hgl::audio
{
    VoiceCall::VoiceCall()
    {
        seq=0;
        sample_rate=0;
        frame_samples=0;
        loss_rate=0.0f;
        packet_buf.resize(4000);
        capture=nullptr;
        policy=nullptr;
        out_cb=nullptr;
        out_user=nullptr;
        device_active=false;
    }

    bool VoiceCall::Start(const OSString &plugin_name,uint sr,uint channels,uint bitrate,uint frame_ms)
    {
        Stop();

        frame_samples=sr*frame_ms/1000;

        if(!codec.Open(plugin_name,sr,channels,bitrate))
            return(false);

        preprocess.Init(sr,frame_samples);

        sample_rate=sr;
        seq=0;

        if(policy)
            policy->RequestFocus();

        return(true);
    }

    void VoiceCall::Stop()
    {
        if(policy)
            policy->AbandonFocus();

        codec.Close();
        jitter.Reset();
        sample_rate=0;
        frame_samples=0;
        seq=0;
        device_active=false;
    }

    bool VoiceCall::UpdateDevice()
    {
        if(!codec.IsOpen()||!capture)
            return(false);

        // 会话策略联动：失焦/被静音 → 输出静音帧（通话挂起），跳过采集
        if(policy)
        {
            policy->Update();

            if(policy->GetFocusState()!=AudioFocusState::HasFocus||policy->IsSilenced())
            {
                device_active=false;

                if(out_cb)
                {
                    std::vector<float> silence(frame_samples,0.0f);
                    out_cb(silence.data(),frame_samples,out_user);
                }

                return(true);
            }
        }

        // 采集一帧（int16 → float）
        std::vector<int16_t> raw(frame_samples);
        const int bytes=capture->ReadFrame(raw.data(),(int)frame_samples);

        if(bytes<=0)
            return(false);

        const int got=bytes/2;

        if((uint)got!=frame_samples)
            return(false);

        std::vector<float> pcm(frame_samples);
        for(uint i=0;i<frame_samples;i++)
            pcm[i]=raw[i]/32768.0f;

        // 发送 + 接收
        Send(pcm.data(),frame_samples);

        std::vector<float> out(frame_samples);
        Receive(out.data(),frame_samples);

        if(out_cb)
            out_cb(out.data(),frame_samples,out_user);

        device_active=true;
        return(true);
    }

    bool VoiceCall::Send(const float *pcm,uint frames)
    {
        if(!codec.IsOpen()||!pcm||frames!=frame_samples)
            return(false);

        // 1. 预处理：NS → AGC，同时得 VAD
        std::vector<float> clean(frame_samples);
        const bool speech=preprocess.Process(pcm,clean.data(),frame_samples);

        // 2. 编码
        const int n=codec.Encode(clean.data(),frame_samples,packet_buf.data(),(uint)packet_buf.size());

        if(n>0)
        {
            // 3. 入抖动缓冲（本机环回：发送即入接收端缓冲）
            jitter.Push(seq,packet_buf.data(),n);
        }

        seq++;
        return(speech);
    }

    bool VoiceCall::Receive(float *out,uint frames)
    {
        if(!codec.IsOpen()||!out||frames!=frame_samples)
            return(false);

        // 模拟网络丢包：以 loss_rate 概率整帧丢弃（不 Poll，播放时钟推进 → PLC）
        if(loss_rate>0.0f)
        {
            static thread_local std::mt19937 rng(0x5EED);
            std::uniform_real_distribution<float> dist(0.0f,1.0f);

            if(dist(rng)<loss_rate)
            {
                codec.Decode(nullptr,0,out,frames);     // PLC
                return(true);
            }
        }

        // 1. 从抖动缓冲取包
        const int n=jitter.Poll(packet_buf.data(),(int)packet_buf.size());

        // 2. 解码（无包 → PLC）
        codec.Decode((n>0)?packet_buf.data():nullptr,n,out,frames);

        return(true);
    }
}//namespace hgl::audio
