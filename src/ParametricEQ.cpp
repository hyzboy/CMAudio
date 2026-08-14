#include<hgl/audio/ParametricEQ.h>

namespace hgl::audio
{
    ParametricEQ::ParametricEQ()
    {
        sample_rate = 48000.0f;
    }

    ParametricEQ::ParametricEQ(float sr)
    {
        sample_rate = sr;
    }

    void ParametricEQ::SetSampleRate(float sr)
    {
        sample_rate = sr;

        for(size_t i = 0; i < bands.size(); i++)
            stages[i].Configure(bands[i].type, sr, bands[i].frequency, bands[i].q, bands[i].gain_db);
    }

    int ParametricEQ::AddBand(BiquadType type, float frequency, float q, float gain_db)
    {
        Band b;

        b.type      = type;
        b.frequency = frequency;
        b.q         = q;
        b.gain_db   = gain_db;

        bands.push_back(b);
        stages.push_back(BiquadFilter(type, sample_rate, frequency, q, gain_db));

        return (int)bands.size() - 1;
    }

    bool ParametricEQ::SetBand(int index, BiquadType type, float frequency, float q, float gain_db)
    {
        if(index < 0 || index >= (int)bands.size())
            return false;

        Band &b = bands[index];

        b.type      = type;
        b.frequency = frequency;
        b.q         = q;
        b.gain_db   = gain_db;

        stages[index].Configure(type, sample_rate, frequency, q, gain_db);

        return true;
    }

    bool ParametricEQ::RemoveBand(int index)
    {
        if(index < 0 || index >= (int)bands.size())
            return false;

        bands.erase(bands.begin() + index);
        stages.erase(stages.begin() + index);

        return true;
    }

    void ParametricEQ::ClearBands()
    {
        bands.clear();
        stages.clear();
    }

    const ParametricEQ::Band *ParametricEQ::GetBand(int index)const
    {
        if(index < 0 || index >= (int)bands.size())
            return nullptr;

        return &bands[index];
    }

    void ParametricEQ::Reset()
    {
        for(BiquadFilter &s : stages)
            s.Reset();
    }

    float ParametricEQ::Process(float x)
    {
        for(BiquadFilter &s : stages)
            x = s.Process(x);

        return x;
    }

    void ParametricEQ::Process(float *samples, int count)
    {
        if(!samples || count < 1)
            return;

        for(int i = 0; i < count; i++)
            samples[i] = Process(samples[i]);
    }

    ParametricEQ ParametricEQ::Create3Band(float sample_rate,
                                           float low_freq,  float low_gain_db,
                                           float mid_freq,  float mid_gain_db, float mid_q,
                                           float high_freq, float high_gain_db)
    {
        ParametricEQ eq(sample_rate);

        eq.AddBand(BiquadType::LowShelf,  low_freq,  0.7071f, low_gain_db);
        eq.AddBand(BiquadType::Peaking,   mid_freq,  mid_q,   mid_gain_db);
        eq.AddBand(BiquadType::HighShelf, high_freq, 0.7071f, high_gain_db);

        return eq;
    }
}//namespace hgl::audio
