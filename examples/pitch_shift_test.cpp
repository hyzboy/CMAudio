// AudioMixer Pitch Shift Test
// 验证 P5：ApplyPitchShift 由线性插值升级为 libsamplerate 高质量重采样
// 纯内存合成，无需 WAV/OpenAL 设备
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

static void GenSine16(std::vector<int16_t> &out, float freq, float amp, int count, float sr = 48000.0f)
{
    out.resize(count);
    for(int i = 0; i < count; i++)
        out[i] = (int16_t)(amp * std::sin(2.0f * PI * freq * (float)i / sr) * 32767.0f);
}

static float Int16RMS(const int16_t *samples, int count)
{
    std::vector<float> f(count);
    for(int i = 0; i < count; i++)
        f[i] = samples[i] / 32768.0f;
    return ComputeRMS(f.data(), count);
}

// 主峰频率：转 float 后 FFT 幅度谱，取最大 bin 对应的频率
static float DominantFreq(const int16_t *samples, int count, float sr)
{
    std::vector<float> f(count);
    for(int i = 0; i < count; i++)
        f[i] = samples[i] / 32768.0f;

    int nfft = 1;
    while(nfft < count)
        nfft <<= 1;

    std::vector<float> mag(nfft / 2 + 1);
    if(!ComputeMagnitudeSpectrum(f.data(), count, mag.data(), (int)mag.size()))
        return -1.0f;

    int peakBin = 1;
    for(int i = 2; i < (int)mag.size(); i++)
        if(mag[i] > mag[peakBin])
            peakBin = i;

    return (float)peakBin * sr / (float)nfft;
}

// 变调混音：输入 1kHz 正弦，pitch 变调，返回输出主频/帧数/RMS
static void RunPitch(float pitch, float &outFreq, int &outFrames, float &outRMS)
{
    const float SR = 48000.0f;
    const int N = 48000;
    std::vector<int16_t> sine;
    GenSine16(sine, 1000.0f, 0.5f, N, SR);

    AudioMixer m;
    m.AddSourceAudio(sine.data(), (uint)(N * (int)sizeof(int16_t)), AL_FORMAT_MONO16, (uint)SR);
    m.AddTrack(0, 0.0f, 1.0f, pitch);

    void *o = nullptr;
    uint sz = 0;
    if(!m.Mix(&o, &sz, 0.0f))   // 自动计算时长
    {
        outFreq = -1.0f; outFrames = 0; outRMS = 0.0f;
        return;
    }

    outFrames = (int)(sz / sizeof(int16_t));
    outFreq = DominantFreq((const int16_t*)o, outFrames, SR);
    outRMS = Int16RMS((const int16_t*)o, outFrames);
    delete[] (char*)o;
}

int main()
{
    std::cout << "AudioMixer Pitch Shift Test" << std::endl;
    std::cout << "===========================" << std::endl;

    float freq; int frames; float rms;

    // 1. pitch=1.0 直通
    RunPitch(1.0f, freq, frames, rms);
    Check("pitch=1.0 帧数 == 48000", frames == 48000);
    CheckNear("pitch=1.0 主频 ≈ 1000Hz", freq, 1000.0f, 50.0f);
    CheckNear("pitch=1.0 RMS ≈ 0.3536", rms, 0.3536f, 0.02f);

    // 2. pitch=2.0 升调（输出时长减半）
    RunPitch(2.0f, freq, frames, rms);
    Check("pitch=2.0 帧数 == 24000", frames == 24000);
    CheckNear("pitch=2.0 主频 ≈ 2000Hz", freq, 2000.0f, 60.0f);
    CheckNear("pitch=2.0 RMS ≈ 0.3536", rms, 0.3536f, 0.03f);

    // 3. pitch=0.5 降调（输出时长加倍）
    RunPitch(0.5f, freq, frames, rms);
    Check("pitch=0.5 帧数 == 96000", frames == 96000);
    CheckNear("pitch=0.5 主频 ≈ 500Hz", freq, 500.0f, 40.0f);
    CheckNear("pitch=0.5 RMS ≈ 0.3536", rms, 0.3536f, 0.03f);

    // 4. TPDF 抖动（mt19937 随机源）：开启 dither 后混音成功且 RMS 无明显偏差
    {
        const float SR = 48000.0f;
        const int N = 48000;
        std::vector<int16_t> sine;
        GenSine16(sine, 1000.0f, 0.5f, N, SR);

        AudioMixer m;
        MixerConfig cfg = m.GetConfig();
        cfg.use_dither = true;
        m.SetConfig(cfg);
        m.AddSourceAudio(sine.data(), (uint)(N * (int)sizeof(int16_t)), AL_FORMAT_MONO16, (uint)SR);
        m.AddTrack(0, 0.0f, 1.0f, 1.0f);

        void *o = nullptr;
        uint sz = 0;
        Check("use_dither Mix 成功", m.Mix(&o, &sz, 1.0f));
        const float rmsDither = Int16RMS((const int16_t*)o, (int)(sz / sizeof(int16_t)));
        CheckNear("dither 后 RMS ≈ 0.3536（噪声 ±1 LSB 可忽略）", rmsDither, 0.3536f, 0.01f);
        delete[] (char*)o;
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
