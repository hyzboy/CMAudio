// AudioEngine Update Test
// 验证引擎统一驱动：总线树 + 资源管理集成 + 空间音频世界注册 + Update() 驱动（资源上传 + 世界刷新）
#include <iostream>
#include <cmath>
#include <hgl/platform/Platform.h>
#include <hgl/audio/AudioEngine.h>
#include <hgl/audio/AudioAssetManager.h>
#include <hgl/audio/AudioBuffer.h>
#include <hgl/audio/AudioListener.h>
#include <hgl/audio/SpatialAudioWorld.h>
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
    const int  samples     = sample_rate;
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
    std::cout << "AudioEngine Update Test" << std::endl;
    std::cout << "=======================" << std::endl;

    // 1. 初始化 OpenAL（null 设备，失败回退默认）
    bool al_ready = openal::InitOpenAL(nullptr, "null", false, false);
    if(!al_ready)
        al_ready = openal::InitOpenAL(nullptr, nullptr, false, false);
    Check("InitOpenAL", al_ready);
    if(!al_ready) return 1;

    // 2. 总线树结构
    AudioEngine engine;
    Check("master 有效增益 1.0",     engine.GetMaster()->GetEffectiveGain() == 1.0f);
    Check("music 父为 master",       engine.GetMusic()->GetParent() == engine.GetMaster());
    Check("sfx 父为 master",         engine.GetSFX()->GetParent() == engine.GetMaster());
    Check("ambient 父为 master",     engine.GetAmbient()->GetParent() == engine.GetMaster());
    Check("ui 父为 master",          engine.GetUI()->GetParent() == engine.GetMaster());

    engine.GetMaster()->SetGain(0.5f);
    Check("master.SetGain(0.5) -> music 0.5", engine.GetMusic()->GetEffectiveGain() == 0.5f);
    Check("master.SetGain(0.5) -> sfx 0.5",   engine.GetSFX()->GetEffectiveGain() == 0.5f);
    engine.GetMaster()->SetGain(1.0f);

    // 3. 资源管理集成（Register 注入空 buffer，绕开文件加载）
    AudioAssetManager *am = engine.GetAssetManager();
    Check("GetAssetManager 非空", am != nullptr);

    AudioBuffer *b1 = new AudioBuffer();   // 空 buffer（不碰 OpenAL）
    Check("Register 到 asset_manager", am->Register(OS_TEXT("x"), b1));

    AudioBuffer *r1 = engine.Acquire(OS_TEXT("x"));
    Check("engine.Acquire 命中 == b1", r1 == b1);
    Check("ref_count == 2",            b1->GetRefCount() == 2);
    engine.Release(r1);
    Check("engine.Release 后 ref_count == 1", b1->GetRefCount() == 1);

    // 4. 空间音频世界注册
    Check("初始 GetWorldCount == 0", engine.GetWorldCount() == 0);

    AudioListener listener;
    SpatialAudioWorld world(8, &listener);

    engine.AddWorld(&world);
    Check("AddWorld 后 GetWorldCount == 1", engine.GetWorldCount() == 1);
    engine.AddWorld(&world);   // 重复添加（集合去重）
    Check("重复 AddWorld 仍 == 1",          engine.GetWorldCount() == 1);

    // 5. Update 统一驱动（空场景：资源无任务 + 世界无音源）
    engine.Update(0.1);
    Check("Update 空场景不崩溃", true);

    // 6. 异步加载驱动（engine.Update 驱动 asset_manager 上传）
    Check("生成测试 wav", GenerateTestWav("test_engine.wav"));
    Check("engine.AcquireAsync 提交", engine.AcquireAsync(OS_TEXT("test_engine.wav")));

    int iters = 500;    // 最多 500 * 10ms = 5 秒
    while(engine.GetAssetManager()->IsLoading() && iters-- > 0)
    {
        engine.Update(0.01);            // 统一驱动：上传异步解码结果
        hgl::SleepSecond(0.01);
    }
    engine.Update();

    AudioBuffer *buf = engine.Acquire(OS_TEXT("test_engine.wav"));
    Check("异步加载后 Acquire 命中且已加载", buf != nullptr && buf->IsLoaded());
    if(buf)
    {
        Check("sample_rate == 44100", buf->GetFreq() == 44100);
        engine.Release(buf);
    }

    // 7. 清理
    engine.RemoveWorld(&world);
    Check("RemoveWorld 后 GetWorldCount == 0", engine.GetWorldCount() == 0);

    engine.Release(b1);                 // ref 1 -> 0，卸载
    Check("资源归零后 GetCount == 0", am->GetCount() == 0);

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
