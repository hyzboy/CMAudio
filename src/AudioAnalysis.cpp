#include<hgl/audio/AudioAnalysis.h>

#include<cmath>
#include<vector>

namespace hgl::audio
{
    namespace
    {
        constexpr double PI = 3.141592653589793238462643383279502884;
    }

    // ===== 电平 / 分贝转换 =====
    float LinearToDB(float v)
    {
        if(v <= 0.0f)
            return -120.0f;

        return (float)(20.0 * std::log10((double)v));
    }

    float DBToLinear(float db)
    {
        return (float)std::pow(10.0, (double)db / 20.0);
    }

    // ===== 电平测量 =====
    float ComputeRMS(const float *samples, int count)
    {
        if(!samples || count < 1)
            return 0.0f;

        double sum = 0.0;

        for(int i = 0; i < count; i++)
            sum += (double)samples[i] * (double)samples[i];

        return (float)std::sqrt(sum / (double)count);
    }

    float ComputeRMSdB(const float *samples, int count)
    {
        return LinearToDB(ComputeRMS(samples, count));
    }

    float ComputePeak(const float *samples, int count)
    {
        if(!samples || count < 1)
            return 0.0f;

        float peak = 0.0f;

        for(int i = 0; i < count; i++)
        {
            const float a = std::fabs(samples[i]);

            if(a > peak)
                peak = a;
        }

        return peak;
    }

    float ComputePeakdB(const float *samples, int count)
    {
        return LinearToDB(ComputePeak(samples, count));
    }

    // ===== FFT =====
    bool FFT(float *real, float *imag, int n)
    {
        if(!real || !imag || n < 2 || (n & (n - 1)) != 0)
            return false;

        // bit-reversal permutation
        for(int i = 1, j = 0; i < n; i++)
        {
            int bit = n >> 1;

            for(; (j & bit) != 0; bit >>= 1)
                j ^= bit;

            j ^= bit;

            if(i < j)
            {
                std::swap(real[i], real[j]);
                std::swap(imag[i], imag[j]);
            }
        }

        // butterflies
        for(int len = 2; len <= n; len <<= 1)
        {
            const double ang    = -2.0 * PI / (double)len;
            const double wlen_r = std::cos(ang);
            const double wlen_i = std::sin(ang);

            for(int i = 0; i < n; i += len)
            {
                double wr = 1.0;
                double wi = 0.0;

                for(int k = 0; k < len / 2; k++)
                {
                    const int even = i + k;
                    const int odd  = i + k + len / 2;

                    const double tr = wr * real[odd] - wi * imag[odd];
                    const double ti = wr * imag[odd] + wi * real[odd];

                    real[odd]  = (float)(real[even] - tr);
                    imag[odd]  = (float)(imag[even] - ti);
                    real[even] = (float)(real[even] + tr);
                    imag[even] = (float)(imag[even] + ti);

                    const double nwr = wr * wlen_r - wi * wlen_i;

                    wi = wr * wlen_i + wi * wlen_r;
                    wr = nwr;
                }
            }
        }

        return true;
    }

    // ===== 频谱 =====
    bool ComputeMagnitudeSpectrum(const float *samples, int count,
                                  float *magnitude, int bin_count)
    {
        if(!samples || count < 1 || !magnitude || bin_count < 1)
            return false;

        // 向上取整到 2 的幂
        int n = 1;

        while(n < count)
            n <<= 1;

        const int nyquist_bins = n / 2 + 1;
        const int out_bins     = (bin_count < nyquist_bins) ? bin_count : nyquist_bins;

        std::vector<float> re(n, 0.0f);
        std::vector<float> im(n, 0.0f);

        for(int i = 0; i < count; i++)
            re[i] = samples[i];

        if(!FFT(re.data(), im.data(), n))
            return false;

        const float inv_n = 1.0f / (float)n;

        for(int i = 0; i < out_bins; i++)
        {
            const float mr = re[i] * inv_n;
            const float mi = im[i] * inv_n;

            magnitude[i] = std::sqrt(mr * mr + mi * mi);
        }

        return true;
    }

    // ===== Onset / 节拍检测 =====
    float ComputeSpectralFlux(const float *current, const float *previous, int bin_count)
    {
        if(!current || !previous || bin_count < 1)
            return 0.0f;

        double flux = 0.0;

        for(int i = 0; i < bin_count; i++)
        {
            const float diff = current[i] - previous[i];

            if(diff > 0.0f)
                flux += (double)diff;
        }

        return (float)flux;
    }

    OnsetDetector::OnsetDetector(float onset_ratio)
    {
        ratio           = onset_ratio;
        previous_energy = 0.0f;
        has_previous    = false;
    }

    bool OnsetDetector::Detect(float frame_energy)
    {
        const bool onset = has_previous && (frame_energy > previous_energy * ratio);

        previous_energy = frame_energy;
        has_previous    = true;

        return onset;
    }

    void OnsetDetector::Reset()
    {
        previous_energy = 0.0f;
        has_previous    = false;
    }
}//namespace hgl::audio
