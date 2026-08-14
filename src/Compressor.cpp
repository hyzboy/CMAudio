#include<hgl/audio/Compressor.h>
#include<hgl/audio/Gain.h>

#include<cmath>

namespace hgl::audio
{
    Compressor::Compressor()
    {
        sample_rate       = 48000.0f;
        gain_reduction_db = 0.0f;
        attack_coeff      = 0.0f;
        release_coeff     = 0.0f;
        makeup_linear     = 1.0f;

        Configure(Settings());
    }

    Compressor::Compressor(float sr, const Settings &s)
        : Compressor()
    {
        sample_rate = sr;
        Configure(s);
    }

    void Compressor::SetSampleRate(float sr)
    {
        sample_rate = sr;

        if(sample_rate > 0.0f)
        {
            attack_coeff  = 1.0f - std::exp(-1.0f / (settings.attack_sec  * sample_rate));
            release_coeff = 1.0f - std::exp(-1.0f / (settings.release_sec * sample_rate));
        }
    }

    void Compressor::Configure(const Settings &s)
    {
        settings = s;

        if(settings.ratio < 1.0f)
            settings.ratio = 1.0f;

        makeup_linear = DBToGain(settings.makeup_gain_db);

        SetSampleRate(sample_rate);
    }

    void Compressor::Reset()
    {
        gain_reduction_db = 0.0f;
    }

    float Compressor::Process(float x)
    {
        const float gain_linear = UpdateFromLevel(std::fabs(x)) * makeup_linear;

        return x * gain_linear;
    }

    float Compressor::UpdateFromLevel(float level)
    {
        // 峰值检测（|x|）
        const float level_db = GainToDB(std::fabs(level));

        // 目标增益衰减（超过阈值部分按 ratio 压缩）
        float target_db = 0.0f;

        if(level_db > settings.threshold_db)
        {
            const float over = level_db - settings.threshold_db;
            target_db = -over * (1.0f - 1.0f / settings.ratio);
        }

        // attack/release 平滑（增益进一步下降用 attack，恢复用 release）
        const float coeff = (target_db < gain_reduction_db) ? attack_coeff : release_coeff;

        gain_reduction_db += coeff * (target_db - gain_reduction_db);

        return DBToGain(gain_reduction_db);
    }

    void Compressor::Process(float *samples, int count)
    {
        if(!samples || count < 1)
            return;

        for(int i = 0; i < count; i++)
            samples[i] = Process(samples[i]);
    }
}//namespace hgl::audio
