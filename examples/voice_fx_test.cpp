// 实时变声器测试（P1）
// 验证 WSOLAShifter（变速不变调）→ LinearResampler（重采样）→ PitchShifter（变调不变速）
// → VoiceChain（效果链 + 预设：None/Robot/Helium/MegaPhone/Chorus）
// 纯内存合成，无需 OpenAL 设备
#include <iostream>
#include <cmath>
#include <vector>
#include <hgl/audio/WSOLAShifter.h>
#include <hgl/audio/LinearResampler.h>
#include <hgl/audio/PitchShifter.h>
#include <hgl/audio/VoiceChain.h>
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

// 生成 float 正弦
static void GenSine(std::vector<float> &out, float freq, float amp, int count, float sr = 48000.0f)
{
    out.resize(count);
    for(int i = 0; i < count; i++)
        out[i] = amp * std::sin(2.0f * PI * freq * (float)i / sr);
}

// 主峰频率
static float DominantFreq(const float *samples, int count, float sr)
{
    int nfft = 1;
    while(nfft < count) nfft <<= 1;

    std::vector<float> mag(nfft / 2 + 1);
    if(!ComputeMagnitudeSpectrum(samples, count, mag.data(), (int)mag.size()))
        return -1.0f;

    int peakBin = 1;
    for(int i = 2; i < (int)mag.size(); i++)
        if(mag[i] > mag[peakBin]) peakBin = i;

    return (float)peakBin * sr / (float)nfft;
}

// 读取某流式处理器的全部输出
static int Drain(void (*process)(const float *, int, void *), void *obj,
                 int (*get_count)(const void *), int (*read)(void *, float *, int),
                 const std::vector<float> &in, std::vector<float> &out)
{
    out.clear();
    process(in.data(), (int)in.size(), obj);

    while(get_count(obj) > 0)
    {
        std::vector<float> tmp(4096);
        int n = read(obj, tmp.data(), 4096);
        if(n <= 0) break;
        out.insert(out.end(), tmp.begin(), tmp.begin() + n);
    }
    return (int)out.size();
}

int main()
{
    std::cout << "Voice FX Test (P1)" << std::endl;
    std::cout << "==================" << std::endl;

    const float SR = 48000.0f;
    const int N = 48000;            // 1 秒

    std::vector<float> sine;
    GenSine(sine, 1000.0f, 0.5f, N, SR);

    // ===== 1. WSOLAShifter：变速不变调 =====
    {
        WSOLAShifter ws;
        ws.Init(SR, 0.5f);          // 时长减半（快进 2 倍）

        std::vector<float> out;
        ws.Process(sine.data(), N);
        while(ws.GetOutputCount() > 0)
        {
            std::vector<float> tmp(4096);
            int n = ws.ReadOutput(tmp.data(), 4096);
            out.insert(out.end(), tmp.begin(), tmp.begin() + n);
        }

        const int expected = N / 2;      // 24000
        Check("stretch=0.5 输出 ≈ 24000（±20%）", out.size() > (size_t)(expected * 0.8) && out.size() < (size_t)(expected * 1.2));
        std::cout << "    [INFO] 输出样本数 = " << out.size() << std::endl;

        const float f = DominantFreq(out.data(), (int)out.size(), SR);
        CheckNear("变速后主频 ≈ 1000Hz（音调不变）", f, 1000.0f, 100.0f);
    }

    // ===== 2. LinearResampler：重采样 =====
    {
        LinearResampler rs;
        rs.Init(SR, 0.5f);          // ratio=输出/输入：0.5 → 时长减半、频率 ×2

        std::vector<float> out;
        rs.Process(sine.data(), N);
        while(rs.GetOutputCount() > 0)
        {
            std::vector<float> tmp(4096);
            int n = rs.ReadOutput(tmp.data(), 4096);
            out.insert(out.end(), tmp.begin(), tmp.begin() + n);
        }

        Check("ratio=0.5 输出 ≈ 24000（±10%）", out.size() > 21600 && out.size() < 26400);
        const float f = DominantFreq(out.data(), (int)out.size(), SR);
        CheckNear("重采样 ratio=0.5 后主频 ≈ 2000Hz", f, 2000.0f, 150.0f);
    }

    // ===== 3. PitchShifter：变调不变速 =====
    {
        PitchShifter ps;
        ps.Init(SR, 2.0f);

        std::vector<float> out;
        ps.Process(sine.data(), N);
        while(ps.GetOutputCount() > 0)
        {
            std::vector<float> tmp(4096);
            int n = ps.ReadOutput(tmp.data(), 4096);
            out.insert(out.end(), tmp.begin(), tmp.begin() + n);
        }

        Check("pitch=2 输出 ≈ 48000（时长不变，±15%）", out.size() > 40800 && out.size() < 55200);
        std::cout << "    [INFO] 输出样本数 = " << out.size() << std::endl;
        const float f = DominantFreq(out.data(), (int)out.size(), SR);
        CheckNear("pitch=2 主频 ≈ 2000Hz", f, 2000.0f, 150.0f);
    }
    {
        PitchShifter ps;
        ps.Init(SR, 0.5f);

        std::vector<float> out;
        ps.Process(sine.data(), N);
        while(ps.GetOutputCount() > 0)
        {
            std::vector<float> tmp(4096);
            int n = ps.ReadOutput(tmp.data(), 4096);
            out.insert(out.end(), tmp.begin(), tmp.begin() + n);
        }

        const float f = DominantFreq(out.data(), (int)out.size(), SR);
        CheckNear("pitch=0.5 主频 ≈ 500Hz", f, 500.0f, 80.0f);
    }

    // ===== 4. VoiceChain 预设 =====
    {
        // None：直通
        {
            VoiceChain vc;
            vc.Init(SR);
            vc.SetPreset(VoicePreset::None);

            std::vector<float> out;
            vc.Process(sine.data(), N);
            while(vc.GetOutputCount() > 0)
            {
                std::vector<float> tmp(4096);
                int n = vc.ReadOutput(tmp.data(), 4096);
                out.insert(out.end(), tmp.begin(), tmp.begin() + n);
            }

            Check("None 输出非零", out.size() > 0);
            const float rms = ComputeRMS(out.data(), (int)out.size());
            CheckNear("None RMS ≈ 0.3535（直通）", rms, 0.3535f, 0.05f);
            std::cout << "    [INFO] None 输出 = " << out.size() << " 样本, RMS = " << rms << std::endl;
        }

        // Helium：+12 半音升调
        {
            VoiceChain vc;
            vc.Init(SR);
            vc.SetPreset(VoicePreset::Helium);

            std::vector<float> out;
            vc.Process(sine.data(), N);
            while(vc.GetOutputCount() > 0)
            {
                std::vector<float> tmp(4096);
                int n = vc.ReadOutput(tmp.data(), 4096);
                out.insert(out.end(), tmp.begin(), tmp.begin() + n);
            }

            Check("Helium 输出非零", out.size() > 0);
            const float f = DominantFreq(out.data(), (int)out.size(), SR);
            CheckNear("Helium 主频 ≈ 2000Hz（+12 半音）", f, 2000.0f, 200.0f);
        }

        // Robot：环形调制（半波门控）→ 能量显著降低但非零
        {
            VoiceChain vc;
            vc.Init(SR);
            vc.SetPreset(VoicePreset::Robot);

            std::vector<float> out;
            vc.Process(sine.data(), N);
            while(vc.GetOutputCount() > 0)
            {
                std::vector<float> tmp(4096);
                int n = vc.ReadOutput(tmp.data(), 4096);
                out.insert(out.end(), tmp.begin(), tmp.begin() + n);
            }

            const float rms = ComputeRMS(out.data(), (int)out.size());
            Check("Robot 输出非零", rms > 0.05f);
            Check("Robot 能量被门控压低（RMS < 0.3）", rms < 0.30f);
            std::cout << "    [INFO] Robot RMS = " << rms << std::endl;
        }

        // MegaPhone：带通 + 压缩
        {
            VoiceChain vc;
            vc.Init(SR);
            vc.SetPreset(VoicePreset::MegaPhone);

            std::vector<float> out;
            vc.Process(sine.data(), N);
            while(vc.GetOutputCount() > 0)
            {
                std::vector<float> tmp(4096);
                int n = vc.ReadOutput(tmp.data(), 4096);
                out.insert(out.end(), tmp.begin(), tmp.begin() + n);
            }

            const float rms = ComputeRMS(out.data(), (int)out.size());
            Check("MegaPhone 输出非零（1kHz 带通通过）", rms > 0.1f);
            std::cout << "    [INFO] MegaPhone RMS = " << rms << std::endl;
        }

        // Chorus：轻微升调 + 湿信号
        {
            VoiceChain vc;
            vc.Init(SR);
            vc.SetPreset(VoicePreset::Chorus);

            std::vector<float> out;
            vc.Process(sine.data(), N);
            while(vc.GetOutputCount() > 0)
            {
                std::vector<float> tmp(4096);
                int n = vc.ReadOutput(tmp.data(), 4096);
                out.insert(out.end(), tmp.begin(), tmp.begin() + n);
            }

            Check("Chorus 输出非零", out.size() > 0 && ComputeRMS(out.data(), (int)out.size()) > 0.05f);
            const float f = DominantFreq(out.data(), (int)out.size(), SR);
            CheckNear("Chorus 主频 ≈ 1100Hz（1.1x 升调）", f, 1100.0f, 200.0f);
        }
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
