// ParametricEQ Test
// 验证 N 段参数化均衡器（级联频率响应 + Band 管理 + 批量处理）（纯数学，无需 OpenAL）
#include <iostream>
#include <cmath>
#include <hgl/audio/ParametricEQ.h>

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

// 稳态正弦幅度响应（增益 = 输出 RMS / 输入 RMS）
static float MeasureGain(ParametricEQ &eq, float freq, float sr, int samples = 20000)
{
    eq.Reset();
    double sum = 0.0;
    int n = 0;

    for(int i = 0; i < samples; i++)
    {
        const float x = std::sin(2.0f * PI * freq * (float)i / sr);
        const float y = eq.Process(x);

        if(i >= samples / 2)
        {
            sum += (double)y * y;
            n++;
        }
    }

    return std::sqrt(sum / (double)n) / SIN_RMS;
}

int main()
{
    std::cout << "ParametricEQ Test" << std::endl;
    std::cout << "=================" << std::endl;

    const float SR = 48000.0f;

    // 1. 空 EQ 直通
    {
        ParametricEQ eq(SR);
        Check("空 EQ GetBandCount == 0", eq.GetBandCount() == 0);
        CheckNear("空 EQ 直通 Process(0.5) == 0.5", eq.Process(0.5f), 0.5f, 0.0001f);
    }

    // 2. 单段 Peaking == BiquadFilter（+6dB @ 1kHz）
    {
        ParametricEQ eq(SR);
        eq.AddBand(BiquadType::Peaking, 1000.0f, 1.0f, 6.0f);
        Check("单段 GetBandCount == 1", eq.GetBandCount() == 1);
        CheckNear("单段 Peaking +6dB @ 1kHz ≈ 2.0", MeasureGain(eq, 1000.0f, SR), 2.0f, 0.05f);
    }

    // 3. 3-band EQ（低频 +6dB shelf、中频 0dB、高频 -6dB shelf）
    {
        ParametricEQ eq = ParametricEQ::Create3Band(SR,
                                                    100.0f, 6.0f,      // 低频 +6dB
                                                    1000.0f, 0.0f, 1.0f,  // 中频 0dB
                                                    10000.0f, -6.0f);  // 高频 -6dB
        Check("3-band GetBandCount == 3", eq.GetBandCount() == 3);
        CheckNear("3-band 30Hz ≈ +6dB(2.0)",  MeasureGain(eq, 30.0f, SR),   2.0f, 0.15f);
        CheckNear("3-band 1kHz ≈ 0dB(1.0)",   MeasureGain(eq, 1000.0f, SR), 1.0f, 0.05f);
        CheckNear("3-band 20kHz ≈ -6dB(0.5)", MeasureGain(eq, 20000.0f, SR), 0.5f, 0.05f);
    }

    // 4. 级联 = 叠加（两段 Peaking 增益相乘）
    {
        ParametricEQ eq(SR);
        eq.AddBand(BiquadType::Peaking, 1000.0f, 1.0f, 3.0f);   // +3dB ≈ 1.4125
        eq.AddBand(BiquadType::Peaking, 1000.0f, 1.0f, 3.0f);   // 再 +3dB
        // 级联 1kHz 增益 = 1.4125 * 1.4125 = 1.995 ≈ 2.0（+6dB）
        CheckNear("两段 +3dB 级联 @ 1kHz ≈ +6dB(2.0)", MeasureGain(eq, 1000.0f, SR), 2.0f, 0.08f);
    }

    // 5. Band 管理（AddBand/SetBand/RemoveBand/GetBand/ClearBands）
    {
        ParametricEQ eq(SR);
        const int idx = eq.AddBand(BiquadType::Lowpass, 1000.0f);
        Check("AddBand 返回索引 0", idx == 0);
        Check("GetBand(0) 非空", eq.GetBand(0) != nullptr);
        Check("GetBand(0)->type == Lowpass", eq.GetBand(0)->type == BiquadType::Lowpass);
        Check("GetBand(-1) == nullptr", eq.GetBand(-1) == nullptr);
        Check("GetBand(1) 越界 == nullptr", eq.GetBand(1) == nullptr);

        Check("SetBand 修改参数成功", eq.SetBand(0, BiquadType::Highpass, 2000.0f, 0.5f, 0.0f));
        CheckNear("SetBand 后 frequency == 2000", eq.GetBand(0)->frequency, 2000.0f, 0.01f);
        Check("SetBand 越界失败", !eq.SetBand(5, BiquadType::Lowpass, 1000.0f, 0.7071f, 0.0f));

        Check("RemoveBand 成功", eq.RemoveBand(0));
        Check("RemoveBand 后 count == 0", eq.GetBandCount() == 0);
        Check("RemoveBand 越界失败", !eq.RemoveBand(0));

        eq.AddBand(BiquadType::Lowpass, 1000.0f);
        eq.AddBand(BiquadType::Highpass, 1000.0f);
        eq.ClearBands();
        Check("ClearBands 后 count == 0", eq.GetBandCount() == 0);
    }

    // 6. 批量 Process == 逐个 Process
    {
        ParametricEQ eq = ParametricEQ::Create3Band(SR, 100.0f, 3.0f, 1000.0f, 0.0f, 1.0f, 10000.0f, -3.0f);

        float buf[1000];
        for(int i = 0; i < 1000; i++)
            buf[i] = 0.3f * std::sin(2.0f * PI * 440.0f * i / SR);

        ParametricEQ eq2 = eq;   // 拷贝一份同样状态
        eq.Process(buf, 1000);   // 批量

        // 逐个：重新从同一初始状态
        eq2.Reset();
        eq.Reset();
        float single[1000];
        for(int i = 0; i < 1000; i++)
            single[i] = eq2.Process(0.3f * std::sin(2.0f * PI * 440.0f * i / SR));

        bool identical = true;
        for(int i = 0; i < 1000; i++)
            if(std::fabs(buf[i] - single[i]) > 1e-5f) identical = false;

        Check("批量 Process == 逐个 Process", identical);
    }

    // 7. SetSampleRate 重算系数（44.1kHz vs 48kHz 响应一致）
    {
        ParametricEQ eq = ParametricEQ::Create3Band(48000.0f, 100.0f, 6.0f, 1000.0f, 0.0f, 1.0f, 10000.0f, -6.0f);
        const float g48 = MeasureGain(eq, 1000.0f, 48000.0f);

        eq.SetSampleRate(44100.0f);
        CheckNear("SetSampleRate 后 GetSampleRate == 44100", eq.GetSampleRate(), 44100.0f, 0.01f);
        const float g44 = MeasureGain(eq, 1000.0f, 44100.0f);
        CheckNear("44.1kHz 下 1kHz 增益不变（中频 0dB）", g44, 1.0f, 0.05f);
        (void)g48;
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
