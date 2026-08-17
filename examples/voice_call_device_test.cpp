// Voice Call Device Test (P4)
// 验证设备模式集成：CaptureSource（真实录音/mock）→ VoiceCall 全链 → 输出回调
// + AudioSessionPolicy 焦点联动（失焦挂起/恢复继续）
// mock 模式无需设备；真实录音设备存在时自动用真实采集
#include <iostream>
#include <cmath>
#include <vector>
#include <cstring>
#include <hgl/audio/VoiceCall.h>
#include <hgl/audio/CaptureSource.h>
#include <hgl/audio/AudioSessionPolicy.h>
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

// 输出回调收集器
struct Collector
{
    std::vector<float> pcm;
    uint frame_samples = 0;
    bool silent_frames = false;     // 是否出现全静音帧（焦点挂起）
};

static void CollectCB(const float *pcm,uint frames,void *user)
{
    Collector *c = (Collector*)user;
    c->pcm.insert(c->pcm.end(), pcm, pcm + frames);

    float sum = 0;
    for(uint i = 0; i < frames; i++)
        sum += pcm[i] * pcm[i];
    if(sum < 1e-9f)
        c->silent_frames = true;
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
    std::cout << "== Voice Call Device Test (P4: 设备集成 + 会话策略) ==" << std::endl;

    const uint sr = 16000;
    const uint frame_samples = 320;     // 20ms @16k

    // ---- 1. mock 设备环回：1000Hz 正弦经全链输出主频保持 ----
    std::cout << "[1] mock 设备环回（CaptureSource→VoiceCall→回调）" << std::endl;
    {
        CaptureSource src;
        Check("CaptureSource mock Open", src.Open(sr, 20, AL_FORMAT_MONO16, true));
        Check("Start", src.Start());

        VoiceCall call;
        Check("VoiceCall Start(Opus)", call.Start(OS_TEXT("Opus"), sr, 1, 32000, 20));

        Collector col;
        call.AttachCapture(&src);
        call.SetOutputCallback(CollectCB, &col);

        // 驱动 60 帧（1.2 秒）
        int done = 0;
        for(int f = 0; f < 60; f++)
            if(call.UpdateDevice())
                done++;

        Check("60 帧全部驱动成功", done == 60);

        // mock 是 1000Hz 正弦，经全链（NS/AGC/Opus/PLC）主频保持
        const float freq_out = DominantFreq(col.pcm.data(), (int)col.pcm.size(), (float)sr);
        CheckNear("设备环回主频 = 1000Hz（mock 信号）", freq_out, 1000.0f, 2.0f);

        Check("输出有能量", ComputeRMS(col.pcm.data(), (int)col.pcm.size()) > 0.05f);

        call.Stop();
        src.Stop();
        src.Close();
    }

    // ---- 2. 会话策略联动：失焦挂起（静音帧）→ 恢复继续 ----
    std::cout << "[2] AudioSessionPolicy 焦点联动" << std::endl;
    {
        DesktopSessionPolicy policy;
        Check("Policy Initialize", policy.Initialize());
        Check("初始 HasFocus", policy.GetFocusState() == AudioFocusState::HasFocus);

        CaptureSource src;
        Check("CaptureSource mock Open", src.Open(sr, 20, AL_FORMAT_MONO16, true));
        src.Start();

        VoiceCall call;
        call.SetSessionPolicy(&policy);
        Check("VoiceCall Start（自动 RequestFocus）", call.Start(OS_TEXT("Opus"), sr, 1, 32000, 20));

        Collector col;
        call.AttachCapture(&src);
        call.SetOutputCallback(CollectCB, &col);

        // 正常通话 20 帧
        for(int f = 0; f < 20; f++)
            call.UpdateDevice();

        // 失去焦点（来电/其他应用抢占）
        policy.AbandonFocus();
        Check("AbandonFocus 后 Lost", policy.GetFocusState() == AudioFocusState::Lost);

        // 挂起期间 20 帧：输出应为静音帧
        col.silent_frames = false;
        const size_t before = col.pcm.size();
        for(int f = 0; f < 20; f++)
            call.UpdateDevice();
        const size_t after = col.pcm.size();

        Check("挂起期间仍输出帧（静音帧）", after > before);
        Check("挂起期间出现静音帧", col.silent_frames);

        // 恢复焦点 → 继续正常采集输出
        policy.RequestFocus();
        Check("RequestFocus 恢复 HasFocus", policy.GetFocusState() == AudioFocusState::HasFocus);

        col.silent_frames = false;
        for(int f = 0; f < 20; f++)
            call.UpdateDevice();

        // 恢复后输出非静音（mock 正弦能量）
        const float rms = ComputeRMS(col.pcm.data() + (int)after, (int)(col.pcm.size() - after));
        CheckNear("恢复后输出有能量（RMS ≈ 0.25 mock 幅度 0.25 的经 AGC 后）", rms, 0.25f, 0.2f);

        call.Stop();    // 自动 AbandonFocus
        Check("Stop 后 Lost（放弃焦点）", policy.GetFocusState() == AudioFocusState::Lost);

        src.Stop();
        src.Close();
    }

    // ---- 3. 真实录音设备（存在时）：采集→环回→输出非静音 ----
    std::cout << "[3] 真实录音设备" << std::endl;
    {
        CaptureSource src;
        const bool real_dev = src.Open(sr, 20, AL_FORMAT_MONO16, false);

        if(real_dev)
        {
            Check("真实设备 Open 成功", true);
            src.Start();

            VoiceCall call;
            Check("VoiceCall Start(Opus)", call.Start(OS_TEXT("Opus"), sr, 1, 32000, 20));

            Collector col;
            call.AttachCapture(&src);
            call.SetOutputCallback(CollectCB, &col);

            int done = 0;
            for(int f = 0; f < 30; f++)     // 0.6 秒真实采集
                if(call.UpdateDevice())
                    done++;

            Check("真实设备驱动 ≥20 帧", done >= 20);

            const float rms = ComputeRMS(col.pcm.data(), (int)col.pcm.size());
            std::cout << "    [INFO] 真实采集输出 RMS=" << rms << "（环境噪声电平，非静音即可）" << std::endl;
            Check("真实设备输出非静音", rms > 1e-4f);

            call.Stop();
            src.Stop();
            src.Close();
        }
        else
        {
            std::cout << "    [SKIP] 无真实录音设备（跳过，mock 已覆盖全链）" << std::endl;
        }
    }

    std::cout << "== 结果: " << (failed ? "FAILED" : "ALL PASSED") << " (" << failed << " failures) ==" << std::endl;
    return failed ? 1 : 0;
}
