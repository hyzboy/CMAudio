// Voice Call Test (P3)
// 验证通话链：JitterBuffer（乱序/丢包/迟到）+ VoiceCall（预处理→Opus 编码→抖动→解码 环回）
// 需要 CMP.Audio.Opus.dll 在运行目录
#include <iostream>
#include <cmath>
#include <vector>
#include <random>
#include <hgl/audio/JitterBuffer.h>
#include <hgl/audio/VoiceCall.h>
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

// 主峰频率
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
    std::cout << "== Voice Call Test (P3: 通话链环回) ==" << std::endl;

    const uint sr = 16000;
    const uint frame_samples = 320;     // 20ms @16k

    // ---- 1. JitterBuffer：乱序重排 / 迟到丢弃 / 丢包 / 上限 ----
    std::cout << "[1] JitterBuffer" << std::endl;
    {
        JitterBuffer jb;
        jb.SetMaxBuffered(5);

        // 乱序到达：首包 0，随后 2,4,1,3（乱序但都在播放时钟之后）
        char pkt[4] = {0,0,0,0};
        jb.Push(0,pkt,4);
        jb.Push(2,pkt,4);
        jb.Push(4,pkt,4);
        jb.Push(1,pkt,4);
        jb.Push(3,pkt,4);
        Check("乱序 5 包缓冲齐", jb.GetBufferedCount()==5);

        // 按序 Poll 出 0..4
        bool in_order=true;
        char out[4];
        for(int i=0;i<5;i++)
        {
            const int n=jb.Poll(out,4);
            if(n!=4) in_order=false;
        }
        Check("乱序后按序输出 5 包", in_order);
        Check("播放时钟推进到 5", jb.GetNextSeq()==5);

        // 丢包：Push 5,7（缺 6）→ Poll 得 5，再 Poll 无帧（PLC 信号）
        jb.Push(5,pkt,4);
        jb.Push(7,pkt,4);
        Check("Poll 得到 seq5", jb.Poll(out,4)==4);
        Check("Poll 丢包（seq6 缺失）返回 0", jb.Poll(out,4)==0);
        Check("播放时钟推进到 7", jb.GetNextSeq()==7);
        Check("迟到包 6 无法再入（已过时钟）", !jb.Push(6,pkt,4));

        // 上限：塞 10 包只留 5
        JitterBuffer jb2;
        jb2.SetMaxBuffered(5);
        for(uint i=0;i<10;i++)
            jb2.Push(i,pkt,4);
        Check("缓冲上限 5（塞 10 只留 5）", jb2.GetBufferedCount()==5);
    }

    // ---- 2. VoiceCall 环回（无丢包）：主频保持 + 时长精确 ----
    std::cout << "[2] VoiceCall 环回（无丢包）" << std::endl;
    {
        VoiceCall call;
        Check("Start(Opus) 成功", call.Start(OS_TEXT("Opus"), sr, 1, 32000, 20));

        const int total_frames = 50;    // 1 秒
        float phase = 0.0f;
        std::vector<float> pcm;
        GenSine(pcm, 440.0f, 0.5f, total_frames * frame_samples, (float)sr, phase);

        // 先发 10 帧静音建立 NS 噪声底（Send+Receive 交替，模拟实时通话节奏）
        std::vector<float> silence(frame_samples, 0.0001f);
        std::vector<float> discard(frame_samples);
        for(int f=0;f<10;f++)
        {
            call.Send(silence.data(), frame_samples);
            call.Receive(discard.data(), frame_samples);
        }

        // 再发语音帧（Send+Receive 交替）
        bool any_speech=false;
        std::vector<float> out(total_frames * frame_samples);
        for(int f=0;f<total_frames;f++)
        {
            if(call.Send(pcm.data()+f*frame_samples, frame_samples))
                any_speech=true;
            call.Receive(out.data()+f*frame_samples, frame_samples);
        }

        Check("VAD 检出语音", any_speech);

        const float freq_out = DominantFreq(out.data(), (int)out.size(), (float)sr);
        CheckNear("环回主频 = 440Hz", freq_out, 440.0f, 2.0f);

        Check("输出非静音", ComputeRMS(out.data(), (int)out.size()) > 0.1f);

        call.Stop();
    }

    // ---- 3. VoiceCall 环回（10% 丢包）：主频保持（PLC 隐藏）----
    std::cout << "[3] VoiceCall 环回（10% 丢包）" << std::endl;
    {
        VoiceCall call;
        Check("Start(Opus) 成功", call.Start(OS_TEXT("Opus"), sr, 1, 32000, 20));
        call.SetLossRate(0.1f);

        const int total_frames = 100;   // 2 秒（丢包 10 帧）
        float phase = 0.0f;
        std::vector<float> pcm;
        GenSine(pcm, 440.0f, 0.5f, total_frames * frame_samples, (float)sr, phase);

        std::vector<float> silence(frame_samples, 0.0001f);
        std::vector<float> discard(frame_samples);
        for(int f=0;f<10;f++)
        {
            call.Send(silence.data(), frame_samples);
            call.Receive(discard.data(), frame_samples);
        }

        std::vector<float> out(total_frames * frame_samples);
        for(int f=0;f<total_frames;f++)
        {
            call.Send(pcm.data()+f*frame_samples, frame_samples);
            call.Receive(out.data()+f*frame_samples, frame_samples);
        }

        const float freq_out = DominantFreq(out.data(), (int)out.size(), (float)sr);
        CheckNear("丢包环回主频仍 ≈ 440Hz", freq_out, 440.0f, 3.0f);

        Check("丢包后输出非静音", ComputeRMS(out.data(), (int)out.size()) > 0.1f);

        call.Stop();
    }

    // ---- 4. VoiceCall 全链：NS 生效（噪声输入 → 输出更干净）----
    std::cout << "[4] 全链降噪效果" << std::endl;
    {
        VoiceCall call;
        Check("Start(Opus) 成功", call.Start(OS_TEXT("Opus"), sr, 1, 32000, 20));

        std::mt19937 rng(7);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        // 纯噪声 20 帧（NS 噪声底预热）
        std::vector<float> noise(frame_samples);
        std::vector<float> discard(frame_samples);
        for(int f=0;f<20;f++)
        {
            for(uint i=0;i<frame_samples;i++)
                noise[i]=dist(rng)*0.1f;
            call.Send(noise.data(), frame_samples);
            call.Receive(discard.data(), frame_samples);
        }

        // 语音+噪声 50 帧
        const int total_frames=50;
        float phase=0.0f;
        std::vector<float> voice;
        GenSine(voice, 440.0f, 0.5f, total_frames*frame_samples, (float)sr, phase);

        std::vector<float> noisy(total_frames*frame_samples);
        for(int i=0;i<total_frames*frame_samples;i++)
            noisy[i]=voice[i]+dist(rng)*0.1f;

        std::vector<float> out(total_frames*frame_samples);
        for(int f=0;f<total_frames;f++)
        {
            call.Send(noisy.data()+f*frame_samples, frame_samples);
            call.Receive(out.data()+f*frame_samples, frame_samples);
        }

        const float freq_out = DominantFreq(out.data(), (int)out.size(), (float)sr);
        CheckNear("噪声环境下环回主频 = 440Hz", freq_out, 440.0f, 2.0f);

        // 2-8kHz 高频噪声带能量应显著低于原始混合信号
        const int fft_n = 16384;
        std::vector<float> mag_in(fft_n/2+1), mag_out(fft_n/2+1);
        ComputeMagnitudeSpectrum(noisy.data(), (int)noisy.size(), mag_in.data(), (int)mag_in.size());
        ComputeMagnitudeSpectrum(out.data(), (int)out.size(), mag_out.data(), (int)mag_out.size());

        float nb=0, na=0;
        for(int b=2048;b<8192;b++)
        {
            nb+=mag_in[b];
            na+=mag_out[b];
        }
        Check("全链输出高频噪声低于输入", na < nb);

        call.Stop();
    }

    std::cout << "== 结果: " << (failed ? "FAILED" : "ALL PASSED") << " (" << failed << " failures) ==" << std::endl;
    return failed ? 1 : 0;
}
