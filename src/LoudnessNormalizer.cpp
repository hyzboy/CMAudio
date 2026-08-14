#include<hgl/audio/LoudnessNormalizer.h>
#include<hgl/audio/AudioAnalysis.h>

#include<cmath>

namespace hgl::audio
{
    namespace
    {
        // EBU R128 K-weighting 标准系数（48kHz，ITU-R BS.1770-4）
        // 高通（pre-filter，衰减低频）
        constexpr float HP_B0 =  1.0f;
        constexpr float HP_B1 = -2.0f;
        constexpr float HP_B2 =  1.0f;
        constexpr float HP_A1 = -1.99004745483398f;
        constexpr float HP_A2 =  0.99007225036621f;

        // 高频 shelf（+4dB 抬升高频）
        constexpr float SHELF_B0 =  1.53512485958697f;
        constexpr float SHELF_B1 = -2.69169618940638f;
        constexpr float SHELF_B2 =  1.19839281085285f;
        constexpr float SHELF_A1 = -1.69065929318241f;
        constexpr float SHELF_A2 =  0.73248077421585f;

        constexpr float LUFS_FLOOR = -70.0f;

        float MeanSquareToLUFS(double ms)
        {
            if(ms <= 1e-10)
                return LUFS_FLOOR;

            const double lufs = 10.0 * std::log10(ms);

            if(lufs < LUFS_FLOOR)
                return LUFS_FLOOR;

            return (float)lufs;
        }
    }

    // ===== KWeightingFilter =====
    KWeightingFilter::KWeightingFilter()
    {
        Reset();
    }

    void KWeightingFilter::Reset()
    {
        stage1.SetCoeffs(HP_B0, HP_B1, HP_B2, HP_A1, HP_A2);
        stage2.SetCoeffs(SHELF_B0, SHELF_B1, SHELF_B2, SHELF_A1, SHELF_A2);
        stage1.Reset();
        stage2.Reset();
    }

    float KWeightingFilter::Process(float x)
    {
        return stage2.Process(stage1.Process(x));
    }

    // ===== LoudnessMeter::RingWindow =====
    void LoudnessMeter::RingWindow::Init(int capacity)
    {
        cap    = capacity;
        buf.assign(cap, 0.0f);
        pos    = 0;
        filled = 0;
        sum    = 0.0;
    }

    void LoudnessMeter::RingWindow::Push(double sq)
    {
        if(filled < cap)
        {
            buf[filled] = (float)sq;
            sum += sq;
            filled++;
        }
        else
        {
            sum -= buf[pos];
            buf[pos] = (float)sq;
            sum += sq;
            pos = (pos + 1) % cap;
        }
    }

    double LoudnessMeter::RingWindow::Mean()const
    {
        return (filled > 0) ? (sum / (double)filled) : 0.0;
    }

    // ===== LoudnessMeter =====
    LoudnessMeter::LoudnessMeter()
    {
        sample_rate      = 0.0f;
        integrated_sum   = 0.0;
        integrated_count = 0;
    }

    bool LoudnessMeter::Init(float rate)
    {
        if(rate != 48000.0f)
            return false;

        sample_rate = rate;
        Reset();
        return true;
    }

    void LoudnessMeter::Reset()
    {
        k_filter.Reset();

        if(sample_rate > 0.0f)
        {
            momentary.Init((int)(0.4 * sample_rate + 0.5));
            short_term.Init((int)(3.0 * sample_rate + 0.5));
        }

        integrated_sum   = 0.0;
        integrated_count = 0;
    }

    void LoudnessMeter::Process(const float *samples, int count)
    {
        if(!samples || count < 1)
            return;

        for(int i = 0; i < count; i++)
        {
            const float  k  = k_filter.Process(samples[i]);
            const double sq = (double)k * (double)k;

            momentary.Push(sq);
            short_term.Push(sq);
            integrated_sum += sq;
            integrated_count++;
        }
    }

    float LoudnessMeter::GetMomentaryLUFS()const
    {
        return MeanSquareToLUFS(momentary.Mean());
    }

    float LoudnessMeter::GetShortTermLUFS()const
    {
        return MeanSquareToLUFS(short_term.Mean());
    }

    float LoudnessMeter::GetIntegratedLUFS()const
    {
        if(integrated_count <= 0)
            return LUFS_FLOOR;

        return MeanSquareToLUFS(integrated_sum / (double)integrated_count);
    }

    // ===== LoudnessNormalizer =====
    float LoudnessNormalizer::ComputeNormalizeGain(const float *samples, int count,
                                                   float sample_rate, float target_lufs)
    {
        LoudnessMeter meter;

        if(!meter.Init(sample_rate))
            return 1.0f;

        meter.Process(samples, count);

        const float current = meter.GetIntegratedLUFS();

        return DBToLinear(target_lufs - current);
    }

    void LoudnessNormalizer::ApplyGain(float *samples, int count, float gain)
    {
        if(!samples || count < 1)
            return;

        for(int i = 0; i < count; i++)
            samples[i] *= gain;
    }
}//namespace hgl::audio
