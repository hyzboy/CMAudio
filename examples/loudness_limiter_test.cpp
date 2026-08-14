// Loudness Limiter Test
// 验证 ApplyPeakLimiter + NormalizeWithLimiter（P3 延伸：响度归一化接 true-peak limiter）
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

static void GenSquare(std::vector<float> &out, float amp, int count)
{
    out.resize(count);
    for(int i = 0; i < count; i++)
        out[i] = (i % 2 == 0) ? amp : -amp;
}

int main()
{
    std::cout << "Loudness Limiter Test" << std::endl;
    std::cout << "=====================" << std::endl;

    const float SR = 48000.0f;

    // 1. ApplyPeakLimiter：1.5 幅度方波（稳态 RMS=1.5）→ 压回 ~1.0
    {
        std::vector<float> sq;
        GenSquare(sq, 1.5f, 48000);

        LoudnessNormalizer::ApplyPeakLimiter(sq.data(), (int)sq.size(), SR, 1.0f);

        // 稳态 RMS（后半段，跳过 attack 收敛期）
        double sum = 0.0;
        for(int i = 24000; i < 48000; i++)
            sum += (double)sq[i] * sq[i];
        const float rms = std::sqrt(sum / 24000.0);

        CheckNear("limiter 稳态 RMS ≈ 1.0（从 1.5 压回）", rms, 1.0f, 0.05f);
        Check("limiter 稳态未超 1.1", rms < 1.1f);
    }

    // 2. NormalizeWithLimiter（limiter 不触发）：满幅 1kHz 正弦 → -23 LUFS
    {
        std::vector<float> sine(144000);
        for(int i = 0; i < 144000; i++)
            sine[i] = std::sin(2.0f * PI * 1000.0f * i / SR);

        LoudnessNormalizer::NormalizeWithLimiter(sine.data(), (int)sine.size(), SR, -23.0f, 1.0f);

        // 验证 LUFS ≈ -23（limiter 不触发，响度正确）
        LoudnessMeter meter;
        meter.Init(SR);
        meter.Process(sine.data(), (int)sine.size());
        CheckNear("NormalizeWithLimiter 后 LUFS ≈ -23", meter.GetIntegratedLUFS(), -23.0f, 0.3f);

        const float peak = ComputePeak(sine.data(), (int)sine.size());
        Check("归一化到 -23 LUFS 峰值远低于 1.0", peak < 0.1f);
    }

    // 3. NormalizeWithLimiter（limiter 触发）：满幅正弦 → 0 LUFS（高增益，峰值本应超 1.0）
    {
        std::vector<float> sine(144000);
        for(int i = 0; i < 144000; i++)
            sine[i] = std::sin(2.0f * PI * 1000.0f * i / SR);

        // 无 limiter 基准：只做增益，峰值 ≈ 1.305
        std::vector<float> no_lim = sine;
        const float gain = LoudnessNormalizer::ComputeNormalizeGain(no_lim.data(), (int)no_lim.size(), SR, 0.0f);
        LoudnessNormalizer::ApplyGain(no_lim.data(), (int)no_lim.size(), gain);
        const float peak_no_lim = ComputePeak(no_lim.data(), (int)no_lim.size());

        // 有 limiter
        LoudnessNormalizer::NormalizeWithLimiter(sine.data(), (int)sine.size(), SR, 0.0f, 1.0f);
        const float peak_lim = ComputePeak(sine.data(), (int)sine.size());

        std::cout << "    (无 limiter 峰值 = " << peak_no_lim << ", 有 limiter 峰值 = " << peak_lim << ")" << std::endl;
        Check("limiter 压低了峰值（< 无 limiter）", peak_lim < peak_no_lim);
        Check("limiter 后峰值 ≤ 1.30", peak_lim <= 1.30f);
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
