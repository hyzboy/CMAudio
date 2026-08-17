#include<hgl/audio/OfflineVoiceFX.h>
#include<hgl/audio/FormantCorrector.h>
#include<hgl/audio/ParametricEQ.h>
#include<hgl/audio/LoudnessNormalizer.h>

namespace hgl::audio
{
    bool OfflineVoiceFX::Process(const float *in,int count,float sample_rate,
                                 const Settings &s,std::vector<float> &out)
    {
        out.clear();

        if(!in||count<=0||sample_rate<=0)
            return false;

        // 1. 变调（WSOLA 大窗 + SINC 重采样）
        OfflinePitchShift ops;

        std::vector<float> shifted;

        if(!ops.Process(in,count,s.pitch,sample_rate,shifted))
            return false;

        // 2. formant 保持（变调后包络搬回原位）
        if(s.preserve_formants&&s.pitch!=1.0f)
        {
            FormantCorrector fc;

            std::vector<float> corrected;

            if(!fc.Process(in,shifted.data(),count,sample_rate,corrected))
                return false;

            shifted.swap(corrected);
        }

        // 3. 可选高频提亮
        if(s.high_shelf_db!=0.0f)
        {
            ParametricEQ eq(sample_rate);

            eq.AddBand(BiquadType::HighShelf,8000.0f,0.7071f,s.high_shelf_db);
            eq.Process(shifted.data(),(int)shifted.size());
        }

        // 4. 可选响度归一化（仅 48kHz 精确支持）
        if(s.target_lufs!=0.0f)
        {
            const float gain=LoudnessNormalizer::ComputeNormalizeGain(
                shifted.data(),(int)shifted.size(),sample_rate,s.target_lufs);

            LoudnessNormalizer::ApplyGain(shifted.data(),(int)shifted.size(),gain);
        }

        out.swap(shifted);

        return true;
    }
}//namespace hgl::audio
