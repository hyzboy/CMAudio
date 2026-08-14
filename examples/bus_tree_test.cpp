// AudioBus Tree Test
// 验证总线树结构、有效增益传播、静音、音源挂载/解挂（纯逻辑，无需 OpenAL 设备与音频文件）
#include <iostream>
#include <hgl/audio/AudioEngine.h>
#include <hgl/audio/AudioSource.h>

using namespace hgl;
using namespace hgl::audio;

static int failed = 0;

static void Check(const char *name, bool cond)
{
    std::cout << (cond ? "  [PASS] " : "  [FAIL] ") << name << std::endl;
    if(!cond) ++failed;
}

static void CheckNear(const char *name, float actual, float expected)
{
    const float diff = actual - expected;
    const bool ok = diff > -0.0001f && diff < 0.0001f;
    std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << name << " = " << actual << " (期望 " << expected << ")" << std::endl;
    if(!ok) ++failed;
}

int main()
{
    std::cout << "AudioBus Tree Test" << std::endl;
    std::cout << "==================" << std::endl;

    // 1. 默认结构
    AudioEngine engine;

    Check("默认 master 有效增益为 1.0", engine.GetMaster()->GetEffectiveGain() == 1.0f);
    Check("默认 music 有效增益为 1.0", engine.GetMusic()->GetEffectiveGain() == 1.0f);
    Check("默认 sfx 有效增益为 1.0",   engine.GetSFX()->GetEffectiveGain() == 1.0f);
    Check("默认 ambient 有效增益为 1.0", engine.GetAmbient()->GetEffectiveGain() == 1.0f);
    Check("默认 ui 有效增益为 1.0",     engine.GetUI()->GetEffectiveGain() == 1.0f);
    Check("music 的父为 master", engine.GetMusic()->GetParent() == engine.GetMaster());
    Check("sfx 的父为 master",   engine.GetSFX()->GetParent() == engine.GetMaster());

    // 2. master 增益传播到整棵树
    engine.GetMaster()->SetGain(0.5f);
    CheckNear("master.SetGain(0.5) -> music",   engine.GetMusic()->GetEffectiveGain(), 0.5f);
    CheckNear("master.SetGain(0.5) -> sfx",     engine.GetSFX()->GetEffectiveGain(), 0.5f);
    CheckNear("master.SetGain(0.5) -> ambient", engine.GetAmbient()->GetEffectiveGain(), 0.5f);
    CheckNear("master.SetGain(0.5) -> ui",      engine.GetUI()->GetEffectiveGain(), 0.5f);

    // 3. 子树增益相乘
    engine.GetSFX()->SetGain(0.5f);
    CheckNear("sfx.SetGain(0.5) -> sfx effective = 0.5*0.5", engine.GetSFX()->GetEffectiveGain(), 0.25f);
    CheckNear("sfx.SetGain(0.5) 不影响 music", engine.GetMusic()->GetEffectiveGain(), 0.5f);

    // 4. 静音传播
    engine.GetSFX()->SetMute(true);
    CheckNear("sfx.SetMute(true) -> sfx effective = 0", engine.GetSFX()->GetEffectiveGain(), 0.0f);
    Check("sfx.IsMute() == true", engine.GetSFX()->IsMute());

    // 恢复
    engine.GetSFX()->SetMute(false);
    engine.GetSFX()->SetGain(1.0f);
    engine.GetMaster()->SetGain(1.0f);
    CheckNear("恢复后 sfx effective = 1.0", engine.GetSFX()->GetEffectiveGain(), 1.0f);
    CheckNear("恢复后 music effective = 1.0", engine.GetMusic()->GetEffectiveGain(), 1.0f);

    // 5. 音源挂载/切换/解挂
    AudioSource src;
    Check("初始 GetBus() == nullptr", src.GetBus() == nullptr);

    src.SetBus(engine.GetSFX());
    Check("挂载后 GetBus() == sfx", src.GetBus() == engine.GetSFX());

    src.SetBus(engine.GetMusic());
    Check("切换后 GetBus() == music", src.GetBus() == engine.GetMusic());

    src.SetBus(nullptr);
    Check("解挂后 GetBus() == nullptr", src.GetBus() == nullptr);

    // 6. 音源自身增益与总线解耦（挂载前后 GetGain 不变，总线只做乘法合成）
    src.SetGain(0.8f);
    src.SetBus(engine.GetSFX());
    CheckNear("挂载后 GetGain() 仍为 0.8（自身增益不变）", src.GetGain(), 0.8f);
    src.SetBus(nullptr);

    // 结果
    std::cout << std::endl;
    if(failed == 0)
    {
        std::cout << "全部通过" << std::endl;
        return 0;
    }

    std::cout << failed << " 项失败" << std::endl;
    return 1;
}
