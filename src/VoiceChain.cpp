#include<hgl/audio/VoiceChain.h>

namespace hgl::audio
{
    VoiceChain::VoiceChain()
    {
        mod_enabled=false;
        mod_freq=80.0f;
        mod_phase=0.0f;
        chorus_enabled=false;
        sample_rate=48000.0f;
        preset=VoicePreset::None;
    }

    void VoiceChain::Init(float sr)
    {
        sample_rate=sr;

        shifter.Init(sr,1.0f);
        eq.SetSampleRate(sr);
        comp.SetSampleRate(sr);
        comp.Configure({-20.0f,1.0f,0.01f,0.1f,0.0f});      // 直通
        chorus.Init(sr,0.020f,0.5f,0.005f,0.0f,0.5f);

        mod_enabled=false;
        mod_freq=80.0f;
        mod_phase=0.0f;
        chorus_enabled=false;

        tmp_buf.resize(2048);
        out_buf.clear();
        out_buf.reserve(4096);

        SetPreset(VoicePreset::None);
    }

    void VoiceChain::SetPitch(float p)
    {
        shifter.SetPitch(p);
    }

    void VoiceChain::Reset()
    {
        shifter.Reset();
        eq.Reset();
        comp.Reset();
        chorus.Reset();

        mod_phase=0.0f;
        out_buf.clear();
    }

    void VoiceChain::SetPreset(VoicePreset p)
    {
        preset=p;

        Reset();

        switch(preset)
        {
            case VoicePreset::None:
                shifter.SetPitch(1.0f);
                eq.ClearBands();
                mod_enabled=false;
                chorus_enabled=false;
                comp.Configure({-20.0f,1.0f,0.01f,0.1f,0.0f});
                break;

            case VoicePreset::Robot:
                shifter.SetPitch(1.0f);
                eq.ClearBands();
                mod_enabled=true;               // 80Hz 方波半波门控
                mod_freq=80.0f;
                chorus_enabled=false;
                comp.Configure({-20.0f,1.0f,0.01f,0.1f,0.0f});
                break;

            case VoicePreset::Helium:
                shifter.SetPitch(2.0f);         // +12 半音
                eq.ClearBands();
                eq.AddBand(BiquadType::HighShelf,8000.0f,0.7071f,6.0f);   // 高频提亮
                mod_enabled=false;
                chorus_enabled=false;
                comp.Configure({-20.0f,1.0f,0.01f,0.1f,0.0f});
                break;

            case VoicePreset::MegaPhone:
                shifter.SetPitch(1.0f);
                eq.ClearBands();
                eq.AddBand(BiquadType::Bandpass,1000.0f,1.0f,0.0f);       // 带通（喇叭感）
                mod_enabled=false;
                chorus_enabled=false;
                comp.Configure({-20.0f,8.0f,0.005f,0.05f,6.0f});          // 高压缩 + 补偿
                break;

            case VoicePreset::Chorus:
                shifter.SetPitch(1.1f);         // 轻微升调
                eq.ClearBands();
                mod_enabled=false;
                chorus_enabled=true;
                comp.Configure({-20.0f,1.0f,0.01f,0.1f,0.0f});
                break;
        }
    }

    void VoiceChain::Process(const float *in,int count)
    {
        if(!in||count<=0)
            return;

        shifter.Process(in,count);

        while(shifter.GetOutputCount()>0)
        {
            const int got=shifter.ReadOutput(tmp_buf.data(),(int)tmp_buf.size());

            if(got<=0)
                break;

            float *p=tmp_buf.data();

            // 1. 环形调制（Robot）：80Hz 方波半波门控
            if(mod_enabled)
            {
                for(int i=0;i<got;i++)
                {
                    p[i]*=(mod_phase<0.5f)?1.0f:0.0f;

                    mod_phase+=mod_freq/sample_rate;

                    if(mod_phase>=1.0f)
                        mod_phase-=1.0f;
                }
            }

            // 2. 参数化 EQ
            if(eq.GetBandCount()>0)
                eq.Process(p,got);

            // 3. Chorus（可选）
            if(chorus_enabled)
                chorus.Process(p,got);

            // 4. 压缩
            comp.Process(p,got);

            out_buf.insert(out_buf.end(),p,p+got);
        }
    }

    int VoiceChain::ReadOutput(float *out,int max_count)
    {
        if(!out||max_count<=0)
            return 0;

        const int n=(max_count<(int)out_buf.size())?max_count:(int)out_buf.size();

        for(int i=0;i<n;i++)
            out[i]=out_buf[i];

        out_buf.erase(out_buf.begin(),out_buf.begin()+n);

        return n;
    }
}//namespace hgl::audio
