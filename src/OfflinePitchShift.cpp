#include<hgl/audio/OfflinePitchShift.h>
#include<hgl/audio/WSOLAShifter.h>
#include<samplerate.h>
#include<cmath>

namespace hgl::audio
{
    OfflinePitchShift::OfflinePitchShift()
    {
    }

    bool OfflinePitchShift::Process(const float *in,int in_count,float pitch,float sample_rate,
                                    std::vector<float> &out)
    {
        out.clear();

        if(!in||in_count<=0||sample_rate<=0)
            return false;

        if(pitch<0.5f)pitch=0.5f;
        if(pitch>2.0f)pitch=2.0f;

        // 1. WSOLA 变速（大窗高质量）：stretch=pitch（拉长），频率不变
        WSOLAShifter ws;

        ws.Init(sample_rate,pitch,60.0f,15.0f);

        std::vector<float> stretched;

        ws.Process(in,in_count);

        while(ws.GetOutputCount()>0)
        {
            std::vector<float> tmp(4096);

            const int got=ws.ReadOutput(tmp.data(),4096);

            if(got<=0)
                break;

            stretched.insert(stretched.end(),tmp.begin(),tmp.begin()+got);
        }

        if(stretched.empty())
            return false;

        // 2. libsamplerate SINC 重采样：ratio=1/pitch（压缩回，频率 ×pitch）
        const double ratio=1.0/pitch;

        const long in_frames=(long)stretched.size();
        const long out_cap=(long)std::ceil((double)in_frames*ratio)+64;

        std::vector<float> out_raw(out_cap);

        SRC_DATA data;
        data.data_in=stretched.data();
        data.data_out=out_raw.data();
        data.input_frames=in_frames;
        data.output_frames=out_cap;
        data.end_of_input=1;
        data.src_ratio=ratio;

        const int result=src_simple(&data,SRC_SINC_MEDIUM_QUALITY,1);

        if(result!=0)
            return false;

        const long gen=data.output_frames_gen;

        if(gen<=0)
            return false;

        // 对齐到输入长度（补零/截断），保证与 FormantCorrector 的帧网格一致
        out.resize(in_count,0.0f);

        const int copy=(gen<(long)in_count)?(int)gen:in_count;

        for(int i=0;i<copy;i++)
            out[i]=out_raw[i];

        return true;
    }
}//namespace hgl::audio
