// Audio Codec Test (P3)
// 验证编码插件接口（ver=5）：AudioCodec 封装 Opus 插件做 PCM↔压缩包 流式编解码
// 需要 CMP.Audio.Opus.dll 在运行目录（build/out/Windows_64_Debug/）
#include <iostream>
#include <cmath>
#include <vector>
#include <cstring>
#include <hgl/audio/AudioCodec.h>
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

// 生成 float 正弦帧（连续相位）
static void GenSineFrames(std::vector<float> &out, float freq, float amp, int total_frames, int frame_samples, float sr, float &phase)
{
    out.resize(total_frames * frame_samples);
    for(int i = 0; i < total_frames * frame_samples; i++)
    {
        out[i] = amp * std::sin(2.0f * PI * freq * phase / sr);
        phase += 1.0f;
    }
}

// 主峰频率（FFT 幅度谱最强 bin）
static float DominantFreq(const float *samples, int count, float sr)
{
    int nfft = 1;
    while(nfft < count)
        nfft <<= 1;

    std::vector<float> mag(nfft / 2 + 1);
    if(!ComputeMagnitudeSpectrum(samples, count, mag.data(), (int)mag.size()))
        return -1.0f;

    int best = 0;
    for(int i = 1; i < (int)mag.size(); i++)
        if(mag[i] > mag[best])
            best = i;

    return (float)best * sr / (float)nfft;
}

int main()
{
    std::cout << "== Audio Codec Test (P3: Opus 编码插件接口) ==" << std::endl;

    const uint sr = 16000;      // 通话采样率
    const uint frame_samples = 320;   // 20ms @16k
    const uint channels = 1;
    const uint bitrate = 32000; // 32kbps 语音码率

    // ---- 1. 打开编解码器（Opus 插件 ver=5）----
    std::cout << "[1] AudioCodec Open/Close" << std::endl;
    {
        AudioCodec codec;

        Check("打开失败时应返回 false（不存在的插件）", !codec.Open(OS_TEXT("NoSuchCodec"), sr, channels, bitrate));

        Check("Open(Opus) 成功", codec.Open(OS_TEXT("Opus"), sr, channels, bitrate));
        Check("IsOpen() == true", codec.IsOpen());

        codec.Close();
        Check("Close 后 IsOpen() == false", !codec.IsOpen());
    }

    // ---- 2. 编解码环回：主频保持 + 时长精确 ----
    std::cout << "[2] Encode/Decode 环回" << std::endl;
    {
        AudioCodec codec;
        Check("Open(Opus) 成功", codec.Open(OS_TEXT("Opus"), sr, channels, bitrate));

        const int total_frames = 50;    // 1 秒 = 50 帧 × 20ms
        float phase = 0.0f;
        std::vector<float> pcm;
        GenSineFrames(pcm, 440.0f, 0.5f, total_frames, frame_samples, (float)sr, phase);

        std::vector<char> packet(4000);
        std::vector<float> decoded(total_frames * frame_samples);

        int pcm_written = 0;
        float max_packet = 0;

        for(int f = 0; f < total_frames; f++)
        {
            int n = codec.Encode(pcm.data() + f * frame_samples, frame_samples, packet.data(), (uint)packet.size());

            Check("编码返回正值（包字节数）", n > 0);
            if(n > (int)max_packet) max_packet = (float)n;

            int m = codec.Decode(packet.data(), n, decoded.data() + pcm_written, frame_samples);

            Check("解码返回 320 样本", m == (int)frame_samples);
            pcm_written += m;
        }

        Check("解码总样本数 = 输入", pcm_written == (int)pcm.size());
        CheckNear("压缩包最大字节数 < 400（PCM 320 样本×4B=1280B 的 1/3 以下）", max_packet, 0.0f, 400.0f);

        const float freq_out = DominantFreq(decoded.data(), pcm_written, (float)sr);
        CheckNear("环回主频 = 440Hz", freq_out, 440.0f, 2.0f);

        const float rms_out = ComputeRMS(decoded.data(), pcm_written);
        CheckNear("环回 RMS ≈ 0.35（0.5 幅度正弦理论 0.3536）", rms_out, 0.3536f, 0.02f);

        codec.Close();
    }

    // ---- 3. PLC 丢包隐藏：packet=nullptr 解码不崩、输出长度正确 ----
    std::cout << "[3] PLC（丢包隐藏）" << std::endl;
    {
        AudioCodec codec;
        Check("Open(Opus) 成功", codec.Open(OS_TEXT("Opus"), sr, channels, bitrate));

        const int total_frames = 10;
        float phase = 0.0f;
        std::vector<float> pcm;
        GenSineFrames(pcm, 440.0f, 0.5f, total_frames, frame_samples, (float)sr, phase);

        std::vector<char> packet(4000);
        std::vector<float> decoded(total_frames * frame_samples);

        // 编码前 5 帧，解码时第 6 帧丢包（PLC），之后继续正常解码
        int pcm_written = 0;
        for(int f = 0; f < 5; f++)
        {
            int n = codec.Encode(pcm.data() + f * frame_samples, frame_samples, packet.data(), (uint)packet.size());
            int m = codec.Decode(packet.data(), n, decoded.data() + pcm_written, frame_samples);
            pcm_written += m;
        }

        int m = codec.Decode(nullptr, 0, decoded.data() + pcm_written, frame_samples);
        Check("丢包帧 PLC 解码返回 320 样本", m == (int)frame_samples);
        pcm_written += m;

        for(int f = 6; f < 10; f++)
        {
            int n = codec.Encode(pcm.data() + f * frame_samples, frame_samples, packet.data(), (uint)packet.size());
            int m = codec.Decode(packet.data(), n, decoded.data() + pcm_written, frame_samples);
            pcm_written += m;
        }

        Check("PLC 后总样本数仍 = 输入", pcm_written == (int)pcm.size());
        Check("PLC 输出非静音（有能量）", ComputeRMS(decoded.data(), pcm_written) > 0.01f);

        codec.Close();
    }

    // ---- 4. 不同码率/采样率 ----
    std::cout << "[4] 参数配置" << std::endl;
    {
        AudioCodec codec48;
        Check("Open(Opus) 48kHz 成功", codec48.Open(OS_TEXT("Opus"), 48000, channels, 24000));

        const uint fs48 = 960;  // 20ms @48k
        float phase = 0.0f;
        std::vector<float> pcm;
        GenSineFrames(pcm, 1000.0f, 0.5f, 50, fs48, 48000.0f, phase);   // 50 帧 → nfft=65536，FFT 分辨率 0.73Hz

        std::vector<char> packet(4000);
        std::vector<float> decoded(pcm.size());

        int pcm_written = 0;
        for(int f = 0; f < 50; f++)
        {
            int n = codec48.Encode(pcm.data() + f * fs48, fs48, packet.data(), (uint)packet.size());
            int m = codec48.Decode(packet.data(), n, decoded.data() + pcm_written, fs48);
            Check("48kHz 解码返回 960 样本", m == (int)fs48);
            pcm_written += m;
        }

        Check("48kHz 解码总样本数 = 输入", pcm_written == (int)pcm.size());

        const float freq_out = DominantFreq(decoded.data(), pcm_written, 48000.0f);
        CheckNear("48kHz 环回主频 = 1000Hz", freq_out, 1000.0f, 2.0f);

        codec48.Close();
    }

    std::cout << "== 结果: " << (failed ? "FAILED" : "ALL PASSED") << " (" << failed << " failures) ==" << std::endl;
    return failed ? 1 : 0;
}
