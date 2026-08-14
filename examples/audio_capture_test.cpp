// AudioCapture Test
// 验证录音封装（依赖录音设备；无设备时优雅跳过）
#include <iostream>
#include <hgl/platform/Platform.h>
#include <hgl/audio/AudioCapture.h>
#include <hgl/audio/OpenAL.h>

using namespace hgl;
using namespace hgl::audio;

static int failed = 0;

static void Check(const char *name, bool cond)
{
    std::cout << (cond ? "  [PASS] " : "  [FAIL] ") << name << std::endl;
    if(!cond) ++failed;
}

int main()
{
    std::cout << "AudioCapture Test" << std::endl;
    std::cout << "=================" << std::endl;

    // 初始化 OpenAL 驱动（加载 alcCapture 系列函数指针，无需播放设备）
    const bool al_ready = openal::InitOpenALDriver(nullptr);
    Check("InitOpenALDriver", al_ready);

    if(!al_ready)
    {
        std::cout << std::endl << "OpenAL 驱动不可用" << std::endl;
        return 1;
    }

    AudioCapture capture;

    Check("初始 !IsOpen", !capture.IsOpen());

    const bool opened = capture.Open(44100, AL_FORMAT_MONO16, 1.0);

    if(!opened)
    {
        std::cout << "  (环境无录音设备，跳过录音功能测试)" << std::endl;
        std::cout << "  跳过（无录音设备）" << std::endl;
        openal::CloseOpenAL();
        return 0;
    }

    Check("Open 成功", opened);
    Check("IsOpen", capture.IsOpen());
    Check("GetSampleRate == 44100", capture.GetSampleRate() == 44100);
    Check("GetBufferSize > 0", capture.GetBufferSize() > 0);

    Check("Start", capture.Start());
    Check("GetAvailableSamples >= 0", capture.GetAvailableSamples() >= 0);

    short buf[1024];
    const int read = capture.ReadSamples(buf, 1024);
    Check("ReadSamples >= 0（不崩溃）", read >= 0);

    Check("Stop", capture.Stop());

    capture.Close();
    Check("Close 后 !IsOpen", !capture.IsOpen());

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
