#include<hgl/audio/BiquadFilter.h>

#include<cmath>
#include<algorithm>

namespace hgl::audio
{
    namespace
    {
        constexpr double PI = 3.141592653589793238462643383279502884;
    }

    BiquadFilter::BiquadFilter()
    {
        type        = BiquadType::Lowpass;
        sample_rate = 48000.0f;
        cutoff      = 1000.0f;
        q           = 0.7071f;
        gain_db     = 0.0f;

        SetCoeffs(1.0f, 0.0f, 0.0f, 0.0f, 0.0f);    // 直通
        Reset();
    }

    BiquadFilter::BiquadFilter(BiquadType t, float sr, float f0, float Q, float db)
        : BiquadFilter()
    {
        Configure(t, sr, f0, Q, db);
    }

    void BiquadFilter::Configure(BiquadType t, float sr, float f0, float Q, float db)
    {
        type = t;
        q    = (Q > 0.0f) ? Q : 0.7071f;
        gain_db = db;

        if(sr <= 0.0f || f0 <= 0.0f)
        {
            sample_rate = sr;
            cutoff      = f0;
            SetCoeffs(1.0f, 0.0f, 0.0f, 0.0f, 0.0f);    // 非法参数：直通
            Reset();
            return;
        }

        sample_rate = sr;

        // clamp 截止频率到 (0, Nyquist)
        f0 = std::min(f0, sr * 0.49f);
        f0 = std::max(f0, 1.0f);
        cutoff = f0;

        const double w0    = 2.0 * PI * (double)f0 / (double)sr;
        const double cw    = std::cos(w0);
        const double sw    = std::sin(w0);
        const double A     = std::pow(10.0, (double)db / 40.0);
        const double alpha = sw / (2.0 * (double)q);

        double b0d, b1d, b2d, a0d, a1d, a2d;

        switch(t)
        {
        case BiquadType::Lowpass:
            b0d = (1.0 - cw) / 2.0;
            b1d = 1.0 - cw;
            b2d = (1.0 - cw) / 2.0;
            a0d = 1.0 + alpha;
            a1d = -2.0 * cw;
            a2d = 1.0 - alpha;
            break;

        case BiquadType::Highpass:
            b0d = (1.0 + cw) / 2.0;
            b1d = -(1.0 + cw);
            b2d = (1.0 + cw) / 2.0;
            a0d = 1.0 + alpha;
            a1d = -2.0 * cw;
            a2d = 1.0 - alpha;
            break;

        case BiquadType::Bandpass:
            b0d = alpha;
            b1d = 0.0;
            b2d = -alpha;
            a0d = 1.0 + alpha;
            a1d = -2.0 * cw;
            a2d = 1.0 - alpha;
            break;

        case BiquadType::BandpassCSG:
            b0d = sw / 2.0;
            b1d = 0.0;
            b2d = -sw / 2.0;
            a0d = 1.0 + alpha;
            a1d = -2.0 * cw;
            a2d = 1.0 - alpha;
            break;

        case BiquadType::Notch:
            b0d = 1.0;
            b1d = -2.0 * cw;
            b2d = 1.0;
            a0d = 1.0 + alpha;
            a1d = -2.0 * cw;
            a2d = 1.0 - alpha;
            break;

        case BiquadType::Peaking:
            b0d = 1.0 + alpha * A;
            b1d = -2.0 * cw;
            b2d = 1.0 - alpha * A;
            a0d = 1.0 + alpha / A;
            a1d = -2.0 * cw;
            a2d = 1.0 - alpha / A;
            break;

        case BiquadType::LowShelf:
        {
            const double a_shelf = sw / std::sqrt(2.0);     // S=1（斜率 1）
            const double sqA     = std::sqrt(A);

            b0d = A * ((A + 1.0) - (A - 1.0) * cw + 2.0 * sqA * a_shelf);
            b1d = 2.0 * A * ((A - 1.0) - (A + 1.0) * cw);
            b2d = A * ((A + 1.0) - (A - 1.0) * cw - 2.0 * sqA * a_shelf);
            a0d = (A + 1.0) + (A - 1.0) * cw + 2.0 * sqA * a_shelf;
            a1d = -2.0 * ((A - 1.0) + (A + 1.0) * cw);
            a2d = (A + 1.0) + (A - 1.0) * cw - 2.0 * sqA * a_shelf;
            break;
        }

        case BiquadType::HighShelf:
        {
            const double a_shelf = sw / std::sqrt(2.0);
            const double sqA     = std::sqrt(A);

            b0d = A * ((A + 1.0) + (A - 1.0) * cw + 2.0 * sqA * a_shelf);
            b1d = -2.0 * A * ((A - 1.0) + (A + 1.0) * cw);
            b2d = A * ((A + 1.0) + (A - 1.0) * cw - 2.0 * sqA * a_shelf);
            a0d = (A + 1.0) - (A - 1.0) * cw + 2.0 * sqA * a_shelf;
            a1d = 2.0 * ((A - 1.0) - (A + 1.0) * cw);
            a2d = (A + 1.0) - (A - 1.0) * cw - 2.0 * sqA * a_shelf;
            break;
        }

        case BiquadType::Allpass:
        default:
            b0d = 1.0 - alpha;
            b1d = -2.0 * cw;
            b2d = 1.0 + alpha;
            a0d = 1.0 + alpha;
            a1d = -2.0 * cw;
            a2d = 1.0 - alpha;
            break;
        }

        // 归一化到 a0 = 1
        b0 = (float)(b0d / a0d);
        b1 = (float)(b1d / a0d);
        b2 = (float)(b2d / a0d);
        a1 = (float)(a1d / a0d);
        a2 = (float)(a2d / a0d);

        Reset();
    }

    void BiquadFilter::SetCoeffs(float B0, float B1, float B2, float A1, float A2)
    {
        b0 = B0;
        b1 = B1;
        b2 = B2;
        a1 = A1;
        a2 = A2;
    }

    void BiquadFilter::Reset()
    {
        x1 = 0.0f;
        x2 = 0.0f;
        y1 = 0.0f;
        y2 = 0.0f;
    }

    float BiquadFilter::Process(float x)
    {
        const float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = y;

        return y;
    }
}//namespace hgl::audio
