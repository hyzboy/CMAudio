// Sidechain Duck Test
// 验证 AudioBus 侧链压缩 Duck：侧链电平动态驱动压缩（替代静态 Duck 的 GainRamp 近似）（纯逻辑，无需 OpenAL）
#include <iostream>
#include <cmath>
#include <hgl/audio/AudioBus.h>

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
    std::cout << "Sidechain Duck Test" << std::endl;
    std::cout << "===================" << std::endl;

    AudioBus music;
    AudioBus *child = music.CreateChild("child");

    // 1. 默认未启用侧链
    Check("默认未启用侧链", !music.IsSidechainDuckEnabled());

    // 2. 配置侧链压缩（threshold=-20dB 即幅度 0.1，ratio=4:1）
    music.SetSidechainDuck(48000.0f, -20.0f, 4.0f, 0.001f, 0.05f);
    Check("启用侧链", music.IsSidechainDuckEnabled());
    CheckNear("初始 duck_scale == 1.0", music.GetDuckScale(), 1.0f, 0.0001f);

    // 3. 侧链满幅（level=1.0，0dB）→ 超阈值 20dB → 压缩 -15dB → duck_scale = 10^(-15/20) = 0.1778
    for(int i = 0; i < 48000; i++)
        music.UpdateSidechainDuck(1.0f);
    CheckNear("level=1.0 → duck_scale ≈ 0.1778", music.GetDuckScale(), 0.1778f, 0.005f);
    Check("IsDucked == true", music.IsDucked());

    // 4. 侧链半幅（level=0.5，-6.02dB）→ 超阈值 13.98dB → 压缩 -10.48dB → 0.2992
    for(int i = 0; i < 48000; i++)
        music.UpdateSidechainDuck(0.5f);
    CheckNear("level=0.5 → duck_scale ≈ 0.2992", music.GetDuckScale(), 0.2992f, 0.005f);

    // 5. 侧链低于阈值（level=0.05，-26dB < -20dB）→ 不压缩 → 恢复 1.0
    for(int i = 0; i < 48000; i++)
        music.UpdateSidechainDuck(0.05f);
    CheckNear("level=0.05 → duck_scale 恢复 1.0", music.GetDuckScale(), 1.0f, 0.005f);
    Check("恢复后 !IsDucked", !music.IsDucked());

    // 6. duck 传播到子树（child 有效增益同步压低）
    for(int i = 0; i < 48000; i++)
        music.UpdateSidechainDuck(1.0f);
    CheckNear("子树 child effective ≈ 0.1778", child->GetEffectiveGain(), 0.1778f, 0.005f);

    // 7. 侧链压缩与静态 Duck 共存：静态 Duck 再叠加
    music.Duck(0.5f, 0.0, 0.0);
    CheckNear("静态 Duck(0.5) 叠加后 duck_scale == 0.5", music.GetDuckScale(), 0.5f, 0.0001f);
    music.Unduck(0.0, 0.0);
    for(int i = 0; i < 48000; i++)
        music.UpdateSidechainDuck(1.0f);
    CheckNear("恢复侧链驱动 duck_scale ≈ 0.1778", music.GetDuckScale(), 0.1778f, 0.005f);

    // 8. Disable 恢复
    music.DisableSidechainDuck();
    Check("Disable 后 !enabled", !music.IsSidechainDuckEnabled());
    CheckNear("Disable 后 duck_scale == 1.0", music.GetDuckScale(), 1.0f, 0.0001f);
    CheckNear("Disable 后 child effective == 1.0", child->GetEffectiveGain(), 1.0f, 0.0001f);

    std::cout << std::endl;
    if(failed == 0)
    {
        std::cout << "全部通过" << std::endl;
        return 0;
    }

    std::cout << failed << " 项失败" << std::endl;
    return 1;
}
