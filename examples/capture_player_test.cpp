// Live 录音伪装解码器测试（P0）
// 验证 CaptureSource（mock 模式 + 真实设备路径）与 AudioPlayer 实时源模式（LoadCapture）
// - mock 模式：无录音设备也可完整验证"捕获→播放"管线
// - 真实设备路径：有设备则验证真实采集，无设备优雅跳过
#include <iostream>
#include <cmath>
#include <cstdint>
#include <hgl/platform/Platform.h>
#include <hgl/audio/CaptureSource.h>
#include <hgl/audio/AudioPlayer.h>
#include <hgl/audio/OpenAL.h>
#include <hgl/time/Time.h>

using namespace hgl;
using namespace hgl::audio;

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

int main()
{
    std::cout << "Capture Player Test (P0)" << std::endl;
    std::cout << "========================" << std::endl;

    bool al_ready = openal::InitOpenAL(nullptr, "null", false, false);

    if(!al_ready)
        al_ready = openal::InitOpenAL(nullptr, nullptr, false, false);

    Check("InitOpenAL", al_ready);

    if(!al_ready)
    {
        std::cout << std::endl << "OpenAL 驱动不可用" << std::endl;
        return 1;
    }

    // ===== 1. CaptureSource mock 模式 =====
    {
        CaptureSource src;
        Check("mock Open 成功", src.Open(16000, 20, AL_FORMAT_MONO16, true));
        Check("GetSampleRate == 16000", src.GetSampleRate() == 16000);
        Check("GetFrameSamples == 320 (20ms@16k)", src.GetFrameSamples() == 320);
        Check("GetFrameBytes == 640", src.GetFrameBytes() == 640);
        Check("GetAvailableSamples == 320", src.GetAvailableSamples() == 320);

        int16_t buf[640];
        const int n = src.ReadFrame(buf, 320);
        Check("ReadFrame 返回 640 字节", n == 640);

        // 1kHz 正弦，幅度 0.25：样本 0=0°，样本 4=90°(+0.25)，样本 12=270°(-0.25)
        CheckNear("mock 样本0 ≈ 0（0°）", buf[0] / 32768.0f, 0.0f, 0.02f);
        CheckNear("mock 样本4 ≈ +0.25（90°）", buf[4] / 32768.0f, 0.25f, 0.02f);
        CheckNear("mock 样本12 ≈ -0.25（270°）", buf[12] / 32768.0f, -0.25f, 0.02f);

        // 帧连续性：320 样本 = 20 整周期（16 样本/周期），帧边界相位对齐
        int16_t buf2[640];
        src.ReadFrame(buf2, 320);
        CheckNear("两帧边界相位连续", buf2[0] / 32768.0f, buf[0] / 32768.0f, 0.01f);

        src.Stop();
        src.Close();
        Check("Close 后 IsOpen == false", !src.IsOpen());
    }

    // ===== 2. 真实设备路径（无设备优雅跳过）=====
    {
        CaptureSource src;
        const bool opened = src.Open(16000, 20, AL_FORMAT_MONO16, false);

        std::cout << "  [INFO] 真实录音设备 " << (opened ? "可用" : "不可用（跳过真实采集）") << std::endl;

        if(opened)
        {
            Check("真实设备 Start 成功", src.Start());
            SleepSecond(0.05);
            const int avail = src.GetAvailableSamples();
            Check("真实设备采集到样本", avail > 0);
            src.Stop();
            src.Close();
        }
    }

    // ===== 3. AudioPlayer 实时源模式（mock）=====
    {
        AudioPlayer player;

        Check("LoadCapture(mock) 成功", player.LoadCapture(16000, 20, true));
        Check("GetTotalTime == 0（实时流时长未知）", player.GetTotalTime() == 0.0);

        player.Play(true);
        Check("Play 后状态 == Play", player.GetPlayState() == PlayState::Play);

        SleepSecond(0.15);              // 等待 ~7 帧，验证播放线程持续运行

        Check("播放中状态保持 == Play", player.GetPlayState() == PlayState::Play);
        Check("GetPlayTime 已累计 > 0", player.GetPlayTime() > 0.0);

        player.Pause();
        Check("Pause 后状态 == Pause", player.GetPlayState() == PlayState::Pause);

        player.Resume();
        Check("Resume 后状态 == Play", player.GetPlayState() == PlayState::Play);

        player.Stop();
        Check("Stop 后状态 == None", player.GetPlayState() == PlayState::None);

        // 二次 Play（Playback 重新预填 + 重新开始采集）
        player.Play(true);
        Check("二次 Play 状态 == Play", player.GetPlayState() == PlayState::Play);
        SleepSecond(0.05);
        player.Stop();
        Check("二次 Stop 状态 == None", player.GetPlayState() == PlayState::None);
    }

    // ===== 4. 真实设备 LoadCapture（不崩溃即可，成败皆可）=====
    {
        AudioPlayer player;
        const bool ok = player.LoadCapture(16000, 20, false);
        std::cout << "  [INFO] LoadCapture(真实设备) " << (ok ? "成功" : "失败（无设备，符合预期）") << std::endl;
        if(ok)
            player.Stop();
    }

    openal::CloseOpenAL();

    std::cout << std::endl;
    if(failed == 0)
    {
        std::cout << "全部通过" << std::endl;
        return 0;
    }

    std::cout << failed << " 项失败" << std::endl;
    return 1;
}
