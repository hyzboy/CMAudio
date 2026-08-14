// BiquadFilter Test
// 验证通用双二阶滤波器（RBJ 全类型）：频率响应 + 参数存取 + Reset（纯数学，无需 OpenAL）
#include <iostream>
#include <cmath>
#include <hgl/audio/BiquadFilter.h>

using namespace hgl::audio;

static const float PI = 3.14159265358979323846f;
static const float SIN_RMS = 0.70710678f;    // 满幅正弦 RMS

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

// 稳态正弦幅度响应：输入满幅正弦，返回输出 RMS（预热前半段后测后半段）
static float MeasureSineRMS(BiquadFilter f, float freq, float sr, int samples = 20000)
{
    f.Reset();
    double sum = 0.0;
    int n = 0;

    for(int i = 0; i < samples; i++)
    {
        const float x = std::sin(2.0f * PI * freq * (float)i / sr);
        const float y = f.Process(x);

        if(i >= samples / 2)   // 只测稳态后半段
        {
            sum += (double)y * y;
            n++;
        }
    }

    return std::sqrt(sum / (double)n);
}

// 稳态 DC 增益：输入常数 1.0，收敛后返回输出
static float MeasureDCGain(BiquadFilter f, int samples = 2000)
{
    f.Reset();
    float y = 0.0f;

    for(int i = 0; i < samples; i++)
        y = f.Process(1.0f);

    return y;
}

int main()
{
    std::cout << "BiquadFilter Test" << std::endl;
    std::cout << "=================" << std::endl;

    const float SR = 48000.0f;

    // 1. 直通（恒等系数）
    {
        BiquadFilter pass;
        pass.SetCoeffs(1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        CheckNear("直通 Process(0.5) == 0.5", pass.Process(0.5f), 0.5f, 0.0001f);
        CheckNear("直通 Process(-0.3) == -0.3", pass.Process(-0.3f), -0.3f, 0.0001f);
    }

    // 2. 各类型 DC 增益（精确可预测）
    {
        CheckNear("Lowpass   DC == 1", MeasureDCGain(BiquadFilter(BiquadType::Lowpass, SR, 1000.0f)), 1.0f, 0.001f);
        CheckNear("Highpass  DC == 0", MeasureDCGain(BiquadFilter(BiquadType::Highpass, SR, 1000.0f)), 0.0f, 0.001f);
        CheckNear("Bandpass  DC == 0", MeasureDCGain(BiquadFilter(BiquadType::Bandpass, SR, 1000.0f)), 0.0f, 0.001f);
        CheckNear("BandpassCSG DC == 0", MeasureDCGain(BiquadFilter(BiquadType::BandpassCSG, SR, 1000.0f)), 0.0f, 0.001f);
        CheckNear("Notch     DC == 1", MeasureDCGain(BiquadFilter(BiquadType::Notch, SR, 1000.0f)), 1.0f, 0.001f);
        CheckNear("Peaking(0dB) DC == 1", MeasureDCGain(BiquadFilter(BiquadType::Peaking, SR, 1000.0f, 1.0f, 0.0f)), 1.0f, 0.001f);
        CheckNear("LowShelf(0dB) DC == 1", MeasureDCGain(BiquadFilter(BiquadType::LowShelf, SR, 1000.0f, 1.0f, 0.0f)), 1.0f, 0.001f);
        CheckNear("HighShelf(0dB) DC == 1", MeasureDCGain(BiquadFilter(BiquadType::HighShelf, SR, 1000.0f, 1.0f, 0.0f)), 1.0f, 0.001f);
        CheckNear("Allpass   DC == 1", MeasureDCGain(BiquadFilter(BiquadType::Allpass, SR, 1000.0f)), 1.0f, 0.001f);
    }

    // 3. 低通频率响应（Butterworth Q=0.7071）
    {
        BiquadFilter lp(BiquadType::Lowpass, SR, 1000.0f, 0.7071f);
        CheckNear("低通 100Hz 增益 ≈ 1.0 (0dB)",   MeasureSineRMS(lp, 100.0f, SR)  / SIN_RMS, 1.0f, 0.02f);
        CheckNear("低通 1kHz(cutoff) ≈ 0.7071 (-3dB)", MeasureSineRMS(lp, 1000.0f, SR) / SIN_RMS, 0.7071f, 0.02f);
        Check("低通 10kHz 大幅衰减", MeasureSineRMS(lp, 10000.0f, SR) / SIN_RMS < 0.05f);
    }

    // 4. 高通频率响应（镜像）
    {
        BiquadFilter hp(BiquadType::Highpass, SR, 1000.0f, 0.7071f);
        Check("高通 100Hz 大幅衰减", MeasureSineRMS(hp, 100.0f, SR) / SIN_RMS < 0.05f);
        CheckNear("高通 1kHz(cutoff) ≈ 0.7071 (-3dB)", MeasureSineRMS(hp, 1000.0f, SR) / SIN_RMS, 0.7071f, 0.02f);
        CheckNear("高通 10kHz 增益 ≈ 1.0 (0dB)", MeasureSineRMS(hp, 10000.0f, SR) / SIN_RMS, 1.0f, 0.02f);
    }

    // 5. Peaking +6dB / -6dB @ 1kHz
    {
        BiquadFilter boost(BiquadType::Peaking, SR, 1000.0f, 1.0f, 6.0f);
        CheckNear("Peaking +6dB @ 1kHz ≈ 2.0", MeasureSineRMS(boost, 1000.0f, SR) / SIN_RMS, 2.0f, 0.05f);

        BiquadFilter cut(BiquadType::Peaking, SR, 1000.0f, 1.0f, -6.0f);
        CheckNear("Peaking -6dB @ 1kHz ≈ 0.5", MeasureSineRMS(cut, 1000.0f, SR) / SIN_RMS, 0.5f, 0.02f);
    }

    // 6. 参数存取
    {
        BiquadFilter lp(BiquadType::Lowpass, SR, 2000.0f, 1.5f, -3.0f);
        Check("GetType == Lowpass", lp.GetType() == BiquadType::Lowpass);
        CheckNear("GetCutoff == 2000", lp.GetCutoff(), 2000.0f, 0.01f);
        CheckNear("GetQ == 1.5", lp.GetQ(), 1.5f, 0.001f);
        CheckNear("GetGainDB == -3", lp.GetGainDB(), -3.0f, 0.001f);
    }

    // 7. Reset 清零状态（不改变系数）
    {
        BiquadFilter lp(BiquadType::Lowpass, SR, 1000.0f);
        for(int i = 0; i < 100; i++)
            lp.Process(1.0f);
        lp.Reset();
        CheckNear("Reset 后 Process(0) == 0", lp.Process(0.0f), 0.0f, 0.0001f);
    }

    // 8. 非法参数兜底（cutoff <= 0 → 直通）
    {
        BiquadFilter bad(BiquadType::Lowpass, SR, 0.0f);
        CheckNear("cutoff=0 兜底直通 Process(0.5)==0.5", bad.Process(0.5f), 0.5f, 0.0001f);
    }

    // 9. Configure 重新配置（可复用实例）
    {
        BiquadFilter f;
        f.Configure(BiquadType::Lowpass, SR, 500.0f);
        Check("Configure 后 GetType == Lowpass", f.GetType() == BiquadType::Lowpass);
        CheckNear("Configure 后 GetCutoff == 500", f.GetCutoff(), 500.0f, 0.01f);
        f.Configure(BiquadType::Highpass, SR, 8000.0f);
        Check("重配置后 GetType == Highpass", f.GetType() == BiquadType::Highpass);
        CheckNear("重配置后 GetCutoff == 8000", f.GetCutoff(), 8000.0f, 0.01f);
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
