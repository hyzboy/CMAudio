#include<hgl/audio/AudioEQ.h>

#include<cstdint>

namespace hgl::audio
{
    bool ApplyEQToPCM(void *data, uint size, const AudioDataInfo &info, ParametricEQ &eq)
    {
        if(!data || size == 0)
            return false;

        if(eq.GetBandCount() <= 0)
            return true;    // 无 band = 直通

        const uint channels = info.channels ? info.channels : 1;
        const uint bits     = info.bits_per_sample;
        const uint bytes_per_sample = bits / 8;
        const uint sample_count     = size / bytes_per_sample;

        if(sample_count == 0)
            return false;

        if(info.sample_rate > 0.0f)
            eq.SetSampleRate((float)info.sample_rate);

        if(info.is_float && bits == 32)
        {
            float *p = (float*)data;

            for(uint ch = 0; ch < channels; ch++)
            {
                eq.Reset();

                for(uint i = ch; i < sample_count; i += channels)
                {
                    float y = eq.Process(p[i]);

                    if(y > 1.0f)      y = 1.0f;
                    else if(y < -1.0f) y = -1.0f;

                    p[i] = y;
                }
            }

            return true;
        }

        if(bits == 16)
        {
            int16_t *p = (int16_t*)data;

            for(uint ch = 0; ch < channels; ch++)
            {
                eq.Reset();

                for(uint i = ch; i < sample_count; i += channels)
                {
                    const float x = p[i] / 32768.0f;

                    float y = eq.Process(x);

                    if(y > 1.0f)      y = 1.0f;
                    else if(y < -1.0f) y = -1.0f;

                    p[i] = (int16_t)(y * 32767.0f);
                }
            }

            return true;
        }

        // 其它格式（8bit/int24 等）不支持
        return false;
    }
}//namespace hgl::audio
