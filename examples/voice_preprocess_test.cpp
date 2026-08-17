// Voice Preprocess Test (P3)
// 验证语音预处理三件套：NS（频谱门限降噪）/ AGC（自动增益）/ VAD（语音检测）
// 纯内存合成，无需设备
#include <iostream>
#include <cmath>
#include <vector>
#include <random>
#include <hgl/audio/VoicePreprocess.h>
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

static void GenSine(std::vector<float> &out, float freq, float amp, int count, float sr, float &phase)
{
    out.resize(count);
    for(int i = 0; i < count; i++)
    {
        out[i] = amp * std::sin(2.0f * PI * freq * phase / sr);
        phase += 1.0f;
    }
}

int main()
{
    std::cout << "== Voice Preprocess Test (P3: NS/AGC/VAD) ==" << std::endl;

    const uint sr = 16000;
    const uint frame_samples = 320;     // 20ms @16k
    const int total_frames = 50;        // 1 秒

    // ---- 1. AGC：低音量输入 → 输出 RMS 拉向目标 ----
    std::cout << "[1] AutoGainControl" << std::endl;
    {
        AutoGainControl agc;
        agc.Init(0.1f);                 // 目标 -20dBFS

        float phase = 0.0f;
        std::vector<float> pcm;
        GenSine(pcm, 440.0f, 0.02f, total_frames * frame_samples, (float)sr, phase);   // 低音量 0.02

        std::vector<float> out(total_frames * frame_samples);

        for(int f = 0; f < total_frames; f++)
            agc.Process(pcm.data() + f * frame_samples, out.data() + f * frame_samples, frame_samples);

        const float rms_out = ComputeRMS(out.data(), (int)out.size());
        CheckNear("输出 RMS ≈ 0.1（目标 -20dBFS）", rms_out, 0.1f, 0.02f);

        // 峰值不削波（软限幅内）
        const float peak = ComputePeak(out.data(), (int)out.size());
        Check("输出峰值 ≤ 1.0", peak <= 1.0f);
    }

    // ---- 2. VAD：静音帧 vs 语音帧 ----
    std::cout << "[2] VoiceActivityDetector" << std::endl;
    {
        VoiceActivityDetector vad;
        vad.Init(15.0f, 5);

        // 先喂 10 帧静音建立噪声底
        std::vector<float> silence(frame_samples, 0.001f);
        for(int f = 0; f < 10; f++)
            Check("静音帧判非语音", !vad.Process(silence.data(), frame_samples));

        // 再喂语音帧（幅度 0.5，能量远高于噪声底）
        float phase = 0.0f;
        std::vector<float> speech;
        GenSine(speech, 440.0f, 0.5f, 10 * frame_samples, (float)sr, phase);

        bool any_speech = false;
        for(int f = 0; f < 10; f++)
            if(vad.Process(speech.data() + f * frame_samples, frame_samples))
                any_speech = true;

        Check("语音帧判语音（至少一帧）", any_speech);

        // 语音结束后的 hangover 期间仍判语音
        VoiceActivityDetector vad2;
        vad2.Init(15.0f, 5);
        for(int f = 0; f < 10; f++)
            vad2.Process(silence.data(), frame_samples);
        for(int f = 0; f < 5; f++)
            vad2.Process(speech.data() + f * frame_samples, frame_samples);   // 5 帧语音

        bool still_speech = false;
        for(int f = 0; f < 5; f++)      // 5 帧静音（hangover=5 内）
            if(vad2.Process(silence.data(), frame_samples))
                still_speech = true;

        Check("hangover 期间仍判语音", still_speech);
    }

    // ---- 3. NS：白噪声+正弦 → 输出 SNR 提升 ----
    std::cout << "[3] NoiseSuppressor" << std::endl;
    {
        // 先喂纯噪声帧建立噪声底
        NoiseSuppressor ns;
        ns.Init(sr, frame_samples);

        std::mt19937 rng(12345);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        std::vector<float> noise(frame_samples);
        for(int f = 0; f < 20; f++)     // 20 帧纯噪声 → 噪声底
        {
            for(uint i = 0; i < frame_samples; i++)
                noise[i] = dist(rng) * 0.15f;
            ns.Process(noise.data(), noise.data());
        }

        // 混合信号：0.5 幅度 500Hz（512 点 FFT bin 16 中心，无泄漏）+ 0.15 白噪声（SNR≈+12dB）
        float phase = 0.0f;
        const int nframes = 50;
        std::vector<float> mix(nframes * frame_samples);
        std::vector<float> clean(nframes * frame_samples);

        GenSine(clean, 500.0f, 0.5f, nframes * frame_samples, (float)sr, phase);

        for(int i = 0; i < nframes * frame_samples; i++)
            mix[i] = clean[i] + dist(rng) * 0.15f;

        std::vector<float> denoised(nframes * frame_samples);
        for(int f = 0; f < nframes; f++)
            ns.Process(mix.data() + f * frame_samples, denoised.data() + f * frame_samples);

        // 频域评估：主频（500Hz）峰 vs 高频噪声带（2-8kHz）能量
        const int fft_n = 16384;    // 0.98Hz/bin @16k，500Hz 在 bin 512 附近
        std::vector<float> mag_before(fft_n / 2 + 1), mag_after(fft_n / 2 + 1);
        ComputeMagnitudeSpectrum(mix.data(), (int)mix.size(), mag_before.data(), (int)mag_before.size());
        ComputeMagnitudeSpectrum(denoised.data(), (int)denoised.size(), mag_after.data(), (int)mag_after.size());

        // 主频保持：500Hz 附近 (bin 500±20) 的最强峰
        float peak_before = 0, peak_after = 0;
        for(int b = 480; b <= 520; b++)
        {
            if(mag_before[b] > peak_before) peak_before = mag_before[b];
            if(mag_after[b] > peak_after) peak_after = mag_after[b];
        }
        Check("500Hz 主峰处理后保留（> 处理前 60%）", peak_after > peak_before * 0.6f);

        // 高频噪声带（2kHz-8kHz = bin 2048-8192）能量应显著下降
        float noise_before = 0, noise_after = 0;
        for(int b = 2048; b < 8192; b++)
        {
            noise_before += mag_before[b];
            noise_after += mag_after[b];
        }
        CheckNear("高频噪声带能量下降（after < before×0.5）", noise_after / noise_before, 0.0f, 0.5f);
    }

    // ---- 4. VoicePreprocess 全链：NS→AGC→VAD ----
    std::cout << "[4] VoicePreprocess 全链" << std::endl;
    {
        VoicePreprocess vp;
        vp.Init(sr, frame_samples);

        std::mt19937 rng(99);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        // 静音+噪声 10 帧
        std::vector<float> noisy_silence(frame_samples);
        std::vector<float> out(frame_samples);
        for(int f = 0; f < 10; f++)
        {
            for(uint i = 0; i < frame_samples; i++)
                noisy_silence[i] = dist(rng) * 0.1f;
            vp.Process(noisy_silence.data(), out.data(), frame_samples);
        }

        // 语音（正弦+噪声）10 帧
        float phase = 0.0f;
        std::vector<float> speech;
        GenSine(speech, 440.0f, 0.3f, 10 * frame_samples, (float)sr, phase);

        bool any_speech = false;
        for(int f = 0; f < 10; f++)
        {
            std::vector<float> frame(frame_samples);
            for(uint i = 0; i < frame_samples; i++)
                frame[i] = speech[f * frame_samples + i] + dist(rng) * 0.1f;

            if(vp.Process(frame.data(), out.data(), frame_samples))
                any_speech = true;
        }

        Check("全链 VAD 检出语音", any_speech);

        // 全链输出电平被 AGC 归一
        float phase2 = 0.0f;
        std::vector<float> speech2;
        GenSine(speech2, 440.0f, 0.01f, 30 * frame_samples, (float)sr, phase2);  // 很轻的语音

        std::vector<float> out2(30 * frame_samples);
        for(int f = 0; f < 30; f++)
            vp.Process(speech2.data() + f * frame_samples, out2.data() + f * frame_samples, frame_samples);

        const float rms_out = ComputeRMS(out2.data(), (int)out2.size());
        Check("全链输出 RMS > 输入 RMS（AGC 提升弱信号）", rms_out > 0.01f);
    }

    std::cout << "== 结果: " << (failed ? "FAILED" : "ALL PASSED") << " (" << failed << " failures) ==" << std::endl;
    return failed ? 1 : 0;
}
