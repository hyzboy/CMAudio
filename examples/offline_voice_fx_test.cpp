// 离线高质量变声器测试（P2）
// 验证 OfflinePitchShift（WSOLA+SINC 变调）、FormantCorrector（formant 保持）
// 与 OfflineVoiceFX 完整管线
// 用"谐波 + formant 包络"模拟语音信号
#include <iostream>
#include <cmath>
#include <vector>
#include <hgl/audio/OfflinePitchShift.h>
#include <hgl/audio/WSOLAShifter.h>
#include <hgl/audio/FormantCorrector.h>
#include <hgl/audio/OfflineVoiceFX.h>
#include <hgl/audio/AudioAnalysis.h>

using namespace hgl;
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

// 纯正弦（变调主频验证）
static void GenSine(std::vector<float> &out, float freq, float amp, int count, float sr = 48000.0f)
{
    out.resize(count);
    for(int i = 0; i < count; i++)
        out[i] = amp * std::sin(2.0f * PI * freq * (float)i / sr);
}

// 模拟语音：基频 f0 谐波 × formant 包络（800/1200/2400Hz 共振峰）
static void GenFormantVoice(std::vector<float> &out, float f0, int count, float sr = 48000.0f)
{
    out.resize(count);
    for(int i = 0; i < count; i++)
    {
        const float t = (float)i / sr;
        float s = 0.0f;
        for(int h = 1; h <= 24; h++)
        {
            const float fh = f0 * (float)h;
            const float env = 1.0f
                + 3.0f  * std::exp(-std::pow((fh - 800.0f)  / 150.0f, 2.0f))
                + 2.0f  * std::exp(-std::pow((fh - 1200.0f) / 200.0f, 2.0f))
                + 1.5f  * std::exp(-std::pow((fh - 2400.0f) / 300.0f, 2.0f));
            s += (env / (float)h) * std::sin(2.0f * PI * fh * t);
        }
        out[i] = s * 0.06f;
    }
}

// 基频检测：50-500Hz 范围内最大谱峰
static float FundamentalFreq(const float *samples, int count, float sr)
{
    int nfft = 1;
    while(nfft < count) nfft <<= 1;

    std::vector<float> mag(nfft / 2 + 1);
    if(!ComputeMagnitudeSpectrum(samples, count, mag.data(), (int)mag.size()))
        return -1.0f;

    const int lo = (int)(50.0f * nfft / sr);
    const int hi = (int)(500.0f * nfft / sr);

    int peak = lo;
    for(int i = lo; i <= hi && i < (int)mag.size(); i++)
        if(mag[i] > mag[peak]) peak = i;

    return (float)peak * sr / (float)nfft;
}

// 频带峰值（formant 位置检测用）
static float PeakInBand(const float *samples, int count, float sr, float lo_hz, float hi_hz)
{
    int nfft = 1;
    while(nfft < count) nfft <<= 1;

    std::vector<float> mag(nfft / 2 + 1);
    if(!ComputeMagnitudeSpectrum(samples, count, mag.data(), (int)mag.size()))
        return 0.0f;

    const int lo = (int)(lo_hz * nfft / sr);
    const int hi = (int)(hi_hz * nfft / sr);

    float p = 0.0f;
    for(int i = lo; i <= hi && i < (int)mag.size(); i++)
        if(mag[i] > p) p = mag[i];

    return p;
}

int main()
{
    std::cout << "Offline Voice FX Test (P2)" << std::endl;
    std::cout << "==========================" << std::endl;

    const float SR = 48000.0f;
    const int N = 48000;            // 1 秒

    std::vector<float> voice;
    GenFormantVoice(voice, 200.0f, N, SR);

    {
        const float f0 = FundamentalFreq(voice.data(), N, SR);
        CheckNear("输入基频 ≈ 200Hz", f0, 200.0f, 25.0f);
    }

    // ===== 1. OfflinePitchShift pitch=2（升八度，纯正弦）=====
    {
        std::vector<float> sine;
        GenSine(sine, 200.0f, 0.5f, N, SR);

        OfflinePitchShift ops;
        std::vector<float> out;
        Check("pitch=2 处理成功", ops.Process(sine.data(), N, 2.0f, SR, out));
        Check("输出时长 ≈ 输入（±10%）", out.size() > (size_t)(N * 0.9) && out.size() < (size_t)(N * 1.1));
        const float f0 = FundamentalFreq(out.data(), (int)out.size(), SR);
        CheckNear("pitch=2 基频 ≈ 400Hz", f0, 400.0f, 50.0f);
    }

    // ===== 2. OfflinePitchShift pitch=0.5（降八度，纯正弦）=====
    {
        std::vector<float> sine;
        GenSine(sine, 200.0f, 0.5f, N, SR);

        OfflinePitchShift ops;
        std::vector<float> out;
        Check("pitch=0.5 处理成功", ops.Process(sine.data(), N, 0.5f, SR, out));
        const float f0 = FundamentalFreq(out.data(), (int)out.size(), SR);
        CheckNear("pitch=0.5 基频 ≈ 100Hz", f0, 100.0f, 25.0f);
    }

    // ===== 3. FormantCorrector：formant 保持 =====
    {
        OfflinePitchShift ops;
        std::vector<float> shifted;
        ops.Process(voice.data(), N, 2.0f, SR, shifted);

        // formant 位置检测：pitch=2 后 formant 从 800 搬到 1600
        const float raw_800  = PeakInBand(shifted.data(), N, SR, 700.0f, 1000.0f);
        const float raw_1600 = PeakInBand(shifted.data(), N, SR, 1400.0f, 1800.0f);

        // 校正：formant 回 800Hz
        FormantCorrector fc;
        std::vector<float> corrected;
        Check("FormantCorrector 处理成功", fc.Process(voice.data(), shifted.data(), N, SR, corrected));
        const float cor_800  = PeakInBand(corrected.data(), N, SR, 700.0f, 1000.0f);
        const float cor_1600 = PeakInBand(corrected.data(), N, SR, 1400.0f, 1800.0f);

        std::cout << "    [INFO] 800/1600 峰: 未校正=" << raw_800 << "/" << raw_1600
                  << " 校正后=" << cor_800 << "/" << cor_1600 << std::endl;
        Check("未校正 formant 在 1600（1600 峰 > 800 峰）", raw_1600 > raw_800);
        Check("校正后 formant 回 800（800 峰 > 1600 峰）", cor_800 > cor_1600);
    }

    // ===== 4. OfflineVoiceFX 完整管线 =====
    {
        OfflineVoiceFX fx;
        OfflineVoiceFX::Settings s;
        s.pitch = 2.0f;
        s.preserve_formants = true;
        s.high_shelf_db = 2.0f;
        s.target_lufs = -16.0f;

        std::vector<float> out;
        Check("OfflineVoiceFX 处理成功", fx.Process(voice.data(), N, SR, s, out));
        Check("输出非零", !out.empty() && ComputeRMS(out.data(), (int)out.size()) > 0.01f);
        const float f0 = FundamentalFreq(out.data(), (int)out.size(), SR);
        CheckNear("管线后基频 ≈ 400Hz", f0, 400.0f, 50.0f);

        // 管线内 formant 保持也生效（对照：关闭 formant；关闭响度归一化排除假象）
        OfflineVoiceFX fx2;
        OfflineVoiceFX::Settings s2 = s;
        s2.preserve_formants = false;
        s2.target_lufs = 0.0f;
        OfflineVoiceFX::Settings s3 = s;
        s3.target_lufs = 0.0f;
        std::vector<float> out2, out3;
        fx2.Process(voice.data(), N, SR, s2, out2);
        fx.Process(voice.data(), N, SR, s3, out3);
        const float on_800  = PeakInBand(out3.data(), (int)out3.size(), SR, 700.0f, 1000.0f);
        const float on_1600 = PeakInBand(out3.data(), (int)out3.size(), SR, 1400.0f, 1800.0f);
        const float off_800  = PeakInBand(out2.data(), (int)out2.size(), SR, 700.0f, 1000.0f);
        const float off_1600 = PeakInBand(out2.data(), (int)out2.size(), SR, 1400.0f, 1800.0f);
        Check("管线开启 formant：800 峰 > 1600 峰", on_800 > on_1600);
        Check("管线关闭 formant：1600 峰 > 800 峰", off_1600 > off_800);
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
