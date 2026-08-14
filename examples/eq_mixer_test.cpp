// AudioMixer EQ Integration Test
// 验证 AudioMixer 离线 EQ：混音输出前应用 ParametricEQ（纯内存合成，无需 WAV/OpenAL）
#include <iostream>
#include <cmath>
#include <vector>
#include <cstdint>
#include <hgl/audio/AudioMixer.h>
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

// 生成正弦 int16 PCM
static void GenSine16(std::vector<int16_t> &out, float freq, float amp, int count, float sr = 48000.0f)
{
    out.resize(count);
    for(int i = 0; i < count; i++)
        out[i] = (int16_t)(amp * std::sin(2.0f * PI * freq * (float)i / sr) * 32767.0f);
}

// int16 输出转 float 后算 RMS（与 AudioMixer 的 /32768 约定一致）
static float Int16RMS(const int16_t *samples, int count)
{
    std::vector<float> f(count);
    for(int i = 0; i < count; i++)
        f[i] = samples[i] / 32768.0f;
    return ComputeRMS(f.data(), count);
}

int main()
{
    std::cout << "AudioMixer EQ Integration Test" << std::endl;
    std::cout << "==============================" << std::endl;

    const float SR = 48000.0f;
    const int N = 48000;    // 1 秒

    std::vector<int16_t> sine;
    GenSine16(sine, 1000.0f, 0.5f, N, SR);    // 幅度 0.5 的 1kHz 正弦

    AudioMixer mixer;

    const int src = mixer.AddSourceAudio(sine.data(), (uint)(N * (int)sizeof(int16_t)), AL_FORMAT_MONO16, (uint)SR);
    Check("AddSourceAudio 返回索引 0", src == 0);
    mixer.AddTrack(0, 0.0f, 1.0f, 1.0f);

    // 1. 无 EQ 混音
    void *out = nullptr;
    uint outSize = 0;
    Check("Mix（无 EQ）成功", mixer.Mix(&out, &outSize, 1.0f));

    const float rms_noeq = Int16RMS((const int16_t*)out, (int)(outSize / sizeof(int16_t)));
    CheckNear("无 EQ RMS ≈ 0.3536（0.5 幅度正弦）", rms_noeq, 0.3536f, 0.02f);
    delete[] (char*)out;

    // 2. 加 EQ：Peaking +6dB @ 1kHz
    mixer.GetEQ().AddBand(BiquadType::Peaking, 1000.0f, 1.0f, 6.0f);
    Check("GetEQ GetBandCount == 1", mixer.GetEQ().GetBandCount() == 1);

    out = nullptr; outSize = 0;
    Check("Mix（有 EQ）成功", mixer.Mix(&out, &outSize, 1.0f));

    const float rms_eq = Int16RMS((const int16_t*)out, (int)(outSize / sizeof(int16_t)));
    CheckNear("EQ +6dB 后 RMS ≈ 0.7071", rms_eq, 0.7071f, 0.03f);
    CheckNear("EQ 增益比 ≈ 2.0（+6dB）", rms_eq / rms_noeq, 2.0f, 0.08f);
    delete[] (char*)out;

    // 3. ClearEQ 恢复直通
    mixer.ClearEQ();
    Check("ClearEQ 后 GetBandCount == 0", mixer.GetEQ().GetBandCount() == 0);

    out = nullptr; outSize = 0;
    mixer.Mix(&out, &outSize, 1.0f);

    const float rms_clear = Int16RMS((const int16_t*)out, (int)(outSize / sizeof(int16_t)));
    CheckNear("ClearEQ 后 RMS 恢复 ≈ 无 EQ", rms_clear, rms_noeq, 0.01f);
    delete[] (char*)out;

    // 4. EQ 只影响目标频段（Q=4 窄带：500Hz 受 Peaking@1kHz 影响极小）
    {
        std::vector<int16_t> sine500;
        GenSine16(sine500, 500.0f, 0.5f, N, SR);

        AudioMixer m2;
        m2.AddSourceAudio(sine500.data(), (uint)(N * (int)sizeof(int16_t)), AL_FORMAT_MONO16, (uint)SR);
        m2.AddTrack(0, 0.0f, 1.0f, 1.0f);

        void *o = nullptr; uint sz = 0;
        m2.Mix(&o, &sz, 1.0f);
        const float rms500_noeq = Int16RMS((const int16_t*)o, (int)(sz / sizeof(int16_t)));
        delete[] (char*)o;

        m2.GetEQ().AddBand(BiquadType::Peaking, 1000.0f, 4.0f, 6.0f);   // Q=4 窄带
        o = nullptr; sz = 0;
        m2.Mix(&o, &sz, 1.0f);
        const float rms500_eq = Int16RMS((const int16_t*)o, (int)(sz / sizeof(int16_t)));
        delete[] (char*)o;

        CheckNear("500Hz 受 Peaking@1kHz(Q=4) 影响小（≈1.0）", rms500_eq / rms500_noeq, 1.0f, 0.1f);
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
