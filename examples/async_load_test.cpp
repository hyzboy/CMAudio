// Async Load Test
// 端到端验证异步加载：OpenAL null 设备 + 生成 wav + 后台解码 + 主线程上传 + 缓存登记
#include <iostream>
#include <cmath>
#include <hgl/platform/Platform.h>
#include <hgl/audio/AudioAssetManager.h>
#include <hgl/audio/AudioBuffer.h>
#include <hgl/audio/OpenAL.h>
#include <hgl/time/Time.h>
#include "WavWriter.h"

using namespace hgl;
using namespace hgl::audio;

static int failed = 0;

static void Check(const char *name, bool cond)
{
    std::cout << (cond ? "  [PASS] " : "  [FAIL] ") << name << std::endl;
    if(!cond) ++failed;
}

// 生成 1 秒 440Hz 正弦波，16bit mono 44100Hz
static bool GenerateTestWav(const char *filename)
{
    const uint sample_rate = 44100;
    const int  samples     = sample_rate;          // 1 秒
    short *data = new short[samples];

    for(int i=0;i<samples;i++)
        data[i] = (short)(sin(2.0*3.141592653589793*440.0*i/sample_rate) * 12000.0);

    WavWriter writer;

    if(!writer.Open(filename, AL_FORMAT_MONO16, sample_rate))
    {
        delete[] data;
        return false;
    }

    writer.Write(data, samples*sizeof(short));
    writer.Close();

    delete[] data;
    return true;
}

int main()
{
    std::cout << "Async Load Test" << std::endl;
    std::cout << "===============" << std::endl;

    // 1. 初始化 OpenAL（优先 null 设备，失败回退默认设备）
    bool al_ready = openal::InitOpenAL(nullptr, "null", false, false);

    if(!al_ready)
    {
        std::cout << "  (null 设备失败，回退默认设备)" << std::endl;
        al_ready = openal::InitOpenAL(nullptr, nullptr, false, false);
    }

    Check("InitOpenAL 成功", al_ready);

    if(!al_ready)
    {
        std::cout << std::endl << "OpenAL 初始化失败，无法继续异步加载测试" << std::endl;
        return 1;
    }

    // 2. 生成测试 wav
    Check("生成测试 wav", GenerateTestWav("test_async.wav"));

    // 3. 提交异步加载
    AudioAssetManager am;

    Check("AcquireAsync 提交成功", am.AcquireAsync(OS_TEXT("test_async.wav")));
    Check("提交后 GetPendingCount > 0", am.GetPendingCount() > 0);

    // 4. 轮询 Update 直到加载完成（后台解码 → 主线程上传 → 缓存登记）
    int max_iters = 500;                        // 最多 500 * 10ms = 5 秒

    while(am.IsLoading() && max_iters-- > 0)
    {
        am.Update();
        hgl::SleepSecond(0.01);
    }

    am.Update();                                // 最后一次处理完成队列

    Check("加载完成 !IsLoading", !am.IsLoading());
    Check("GetPendingCount == 0", am.GetPendingCount() == 0);
    Check("缓存已登记 Contains", am.Contains(OS_TEXT("test_async.wav")));
    Check("GetCount == 1", am.GetCount() == 1);

    // 5. Acquire 命中缓存（去重 + 引用计数）
    AudioBuffer *buf = am.Acquire(OS_TEXT("test_async.wav"));

    Check("Acquire 命中缓存", buf != nullptr);
    Check("buffer IsLoaded", buf && buf->IsLoaded());

    if(buf)
    {
        Check("sample_rate == 44100", buf->GetFreq() == 44100);
        Check("duration ≈ 1.0s", buf->GetTime() > 0.9 && buf->GetTime() < 1.1);
        Check("ref_count == 1 (预加载ref0 + Acquire1)", buf->GetRefCount() == 1);
    }

    // 6. 同步 Acquire 同文件命中同一指针（与异步加载的缓存去重一致）
    AudioBuffer *buf2 = am.Acquire(OS_TEXT("test_async.wav"));
    Check("再次 Acquire 命中同一 buffer", buf2 == buf);
    if(buf2)
    {
        Check("ref_count == 2", buf2->GetRefCount() == 2);
        am.Release(buf2);
    }

    // 7. 清理
    am.Release(buf);
    Check("释放全部引用后 GetCount == 0", am.GetCount() == 0);

    am.Clear();
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
