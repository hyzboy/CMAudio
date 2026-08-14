// AudioEQ Test
// 验证 ApplyEQToPCM：对 PCM 数据原地应用参数化 EQ（int16/float32/多声道/直通）（纯 CPU，无需 OpenAL）
#include <iostream>
#include <cmath>
#include <vector>
#include <cstdint>
#include <hgl/audio/AudioEQ.h>
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

static void MakeInfo(AudioDataInfo &info, uint channels, uint bits, bool is_float, uint sr)
{
    info.channels = channels;
    info.bits_per_sample = bits;
    info.is_float = is_float;
    info.sample_rate = sr;
}

int main()
{
    std::cout << "AudioEQ Test" << std::endl;
    std::cout << "============" << std::endl;

    const uint SR = 48000;
    const int N = 48000;    // 1 秒

    // 1. int16 mono：+6dB Peaking @ 1kHz 使 RMS 翻倍
    {
        std::vector<int16_t> pcm(N);
        for(int i = 0; i < N; i++)
            pcm[i] = (int16_t)(0.5f * std::sin(2.0f * PI * 1000.0f * i / SR) * 32767.0f);

        // 原 RMS
        std::vector<float> f(N);
        for(int i = 0; i < N; i++) f[i] = pcm[i] / 32768.0f;
        const float rms_before = ComputeRMS(f.data(), N);

        AudioDataInfo info;
        MakeInfo(info, 1, 16, false, SR);
        info.data_size = N * 2;

        ParametricEQ eq;
        eq.AddBand(BiquadType::Peaking, 1000.0f, 1.0f, 6.0f);

        Check("int16 ApplyEQToPCM 成功", ApplyEQToPCM(pcm.data(), N * 2, info, eq));

        for(int i = 0; i < N; i++) f[i] = pcm[i] / 32768.0f;
        const float rms_after = ComputeRMS(f.data(), N);

        CheckNear("int16 +6dB 后 RMS 翻倍（比≈2.0）", rms_after / rms_before, 2.0f, 0.08f);
    }

    // 2. float32 mono：同样验证
    {
        std::vector<float> pcm(N);
        for(int i = 0; i < N; i++)
            pcm[i] = 0.5f * std::sin(2.0f * PI * 1000.0f * i / SR);

        const float rms_before = ComputeRMS(pcm.data(), N);

        AudioDataInfo info;
        MakeInfo(info, 1, 32, true, SR);

        ParametricEQ eq;
        eq.AddBand(BiquadType::Peaking, 1000.0f, 1.0f, 6.0f);

        Check("float32 ApplyEQToPCM 成功", ApplyEQToPCM(pcm.data(), N * 4, info, eq));

        const float rms_after = ComputeRMS(pcm.data(), N);
        CheckNear("float32 +6dB 后 RMS 翻倍（比≈2.0）", rms_after / rms_before, 2.0f, 0.08f);
    }

    // 3. 直通（无 band）数据不变
    {
        std::vector<int16_t> pcm(N);
        for(int i = 0; i < N; i++)
            pcm[i] = (int16_t)(0.5f * std::sin(2.0f * PI * 1000.0f * i / SR) * 32767.0f);
        std::vector<int16_t> orig = pcm;

        AudioDataInfo info;
        MakeInfo(info, 1, 16, false, SR);

        ParametricEQ eq;   // 空 EQ
        Check("无 band ApplyEQToPCM 返回 true（直通）", ApplyEQToPCM(pcm.data(), N * 2, info, eq));
        Check("直通后数据不变", pcm == orig);
    }

    // 4. 立体声：两声道都翻倍
    {
        std::vector<int16_t> pcm(N * 2);   // 交错 L R L R
        for(int i = 0; i < N; i++)
        {
            const float s = 0.5f * std::sin(2.0f * PI * 1000.0f * i / SR);
            pcm[i * 2 + 0] = (int16_t)(s * 32767.0f);   // L
            pcm[i * 2 + 1] = (int16_t)(s * 32767.0f);   // R
        }

        AudioDataInfo info;
        MakeInfo(info, 2, 16, false, SR);
        info.data_size = N * 2 * 2;

        ParametricEQ eq;
        eq.AddBand(BiquadType::Peaking, 1000.0f, 1.0f, 6.0f);

        Check("stereo ApplyEQToPCM 成功", ApplyEQToPCM(pcm.data(), N * 2 * 2, info, eq));

        // 左右声道 RMS（抽取）
        std::vector<float> fl(N), fr(N);
        for(int i = 0; i < N; i++) { fl[i] = pcm[i*2+0] / 32768.0f; fr[i] = pcm[i*2+1] / 32768.0f; }
        const float rms_l = ComputeRMS(fl.data(), N);
        const float rms_r = ComputeRMS(fr.data(), N);

        CheckNear("stereo 左声道 +6dB 后 RMS ≈ 0.7071", rms_l, 0.7071f, 0.03f);
        CheckNear("stereo 右声道 +6dB 后 RMS ≈ 0.7071", rms_r, 0.7071f, 0.03f);
    }

    // 5. 不支持的格式（8bit）返回 false 不修改
    {
        std::vector<int8_t> pcm(N);
        for(int i = 0; i < N; i++)
            pcm[i] = (int8_t)(0.5f * std::sin(2.0f * PI * 1000.0f * i / SR) * 127.0f);
        std::vector<int8_t> orig = pcm;

        AudioDataInfo info;
        MakeInfo(info, 1, 8, false, SR);

        ParametricEQ eq;
        eq.AddBand(BiquadType::Peaking, 1000.0f, 1.0f, 6.0f);

        Check("8bit 返回 false（不支持）", !ApplyEQToPCM(pcm.data(), N, info, eq));
        Check("8bit 数据未修改", pcm == orig);
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
