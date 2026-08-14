// LoudnessNormalizer Test
// 验证 K-weighting / LUFS 响度测量 / 归一化增益（纯数学，无需 OpenAL）
#include <iostream>
#include <cmath>
#include <vector>
#include <hgl/audio/LoudnessNormalizer.h>
#include <hgl/audio/AudioAnalysis.h>

using namespace hgl::audio;

static const float PI = 3.14159265358979323846f;

static int failed = 0;

static void Check(const char *name, bool cond)
{
    std::cout << (cond ? "  [PASS] " : "  [FAIL] ") << name << std::endl;
    if(!cond) ++failed;
}

static void CheckNear(const char *name, float actual, float expected, float tol)
{
    const bool ok = std::fabs(actual - expected) <= tol;
    std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << name << " = " << actual
              << " (期望 " << expected << " ±" << tol << ")" << std::endl;
    if(!ok) ++failed;
}

static void GenSine(std::vector<float> &out, int count, float freq, float amp, float sr = 48000.0f)
{
    out.resize(count);
    for(int i = 0; i < count; i++)
        out[i] = amp * std::sin(2.0f * PI * freq * (float)i / sr);
}

int main()
{
    std::cout << "LoudnessNormalizer Test" << std::endl;
    std::cout << "========================" << std::endl;

    // 1. BiquadFilter 恒等系数
    {
        BiquadFilter bf;
        bf.SetCoeffs(1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        CheckNear("Biquad 恒等：Process(0.5) == 0.5", bf.Process(0.5f), 0.5f, 0.0001f);
        CheckNear("Biquad 恒等：Process(-0.3) == -0.3", bf.Process(-0.3f), -0.3f, 0.0001f);
    }

    // 2. 采样率校验
    {
        LoudnessMeter m;
        Check("Init(48000) 成功", m.Init(48000.0f));
        LoudnessMeter m2;
        Check("Init(44100) 拒绝（非 48kHz）", !m2.Init(44100.0f));
        Check("Init(96000) 拒绝（非 48kHz）", !m2.Init(96000.0f));
    }

    // 3. 满幅 1kHz 正弦：K-weighting 在 1kHz 增益约 +0.7dB（高频 shelf 已起效）
    //    LUFS = 10*log10(0.5 * 1.084^2) ≈ -2.31
    {
        std::vector<float> s;
        GenSine(s, 144000, 1000.0f, 1.0f);   // 3 秒填满 short-term 窗口
        LoudnessMeter m;
        m.Init(48000.0f);
        m.Process(s.data(), (int)s.size());
        CheckNear("1kHz 满幅 short-term ≈ -2.31 LUFS", m.GetShortTermLUFS(), -2.3126f, 0.1f);
        CheckNear("1kHz 满幅 momentary ≈ -2.31 LUFS",  m.GetMomentaryLUFS(), -2.3126f, 0.1f);
        CheckNear("1kHz 满幅 integrated ≈ -2.31 LUFS", m.GetIntegratedLUFS(), -2.3126f, 0.1f);
    }

    // 4. 半幅 1kHz：LUFS 低 6.02
    {
        std::vector<float> s;
        GenSine(s, 144000, 1000.0f, 0.5f);
        LoudnessMeter m;
        m.Init(48000.0f);
        m.Process(s.data(), (int)s.size());
        CheckNear("0.5 幅度 1kHz short-term ≈ -8.33 LUFS", m.GetShortTermLUFS(), -8.3332f, 0.1f);
    }

    // 5. K-weighting 低频衰减：同幅度 100Hz 响度低于 1kHz
    {
        std::vector<float> lo, hi;
        GenSine(lo, 144000, 100.0f,  1.0f);
        GenSine(hi, 144000, 1000.0f, 1.0f);
        LoudnessMeter mlo, mhi;
        mlo.Init(48000.0f); mlo.Process(lo.data(), (int)lo.size());
        mhi.Init(48000.0f); mhi.Process(hi.data(), (int)hi.size());
        const float l = mlo.GetShortTermLUFS();
        const float h = mhi.GetShortTermLUFS();
        Check("K-weighting 低频衰减（100Hz < 1kHz）", l < h - 0.5f);
        std::cout << "    (100Hz = " << l << " LUFS, 1kHz = " << h << " LUFS)" << std::endl;
    }

    // 6. 静音 -> -70 LUFS
    {
        std::vector<float> s(48000, 0.0f);
        LoudnessMeter m;
        m.Init(48000.0f);
        m.Process(s.data(), (int)s.size());
        CheckNear("静音 short-term == -70 LUFS", m.GetShortTermLUFS(), -70.0f, 0.01f);
    }

    // 7. 归一化增益：满幅 1kHz（-2.31 LUFS）-> 目标 -23 -> gain = 10^(-20.69/20) ≈ 0.092
    {
        std::vector<float> s;
        GenSine(s, 144000, 1000.0f, 1.0f);
        const float gain = LoudnessNormalizer::ComputeNormalizeGain(s.data(), (int)s.size(), 48000.0f, -23.0f);
        CheckNear("归一化增益 ≈ 0.0924", gain, 0.0924f, 0.005f);
    }

    // 8. ApplyGain：应用 0.5 增益后 RMS 减半
    {
        std::vector<float> s, g;
        GenSine(s, 48000, 1000.0f, 0.8f);
        g = s;
        const float before = ComputeRMS(s.data(), (int)s.size());
        LoudnessNormalizer::ApplyGain(g.data(), (int)g.size(), 0.5f);
        const float after = ComputeRMS(g.data(), (int)g.size());
        CheckNear("ApplyGain 后 RMS 减半", after, before * 0.5f, 0.001f);
    }

    std::cout << std::endl;
    if(failed == 0)
    {
        std::cout << "全部通过" << std::endl;
        return 0;
    }

    std::cout << failed << " 项失败" << std::endl;
    return 1;
}
