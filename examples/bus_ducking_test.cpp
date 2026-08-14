// AudioBus Ducking Test
// 验证 Ducking（侧链）：总线音量被平滑压低/恢复，且传播到子树（纯逻辑，无需 OpenAL）
#include <iostream>
#include <hgl/audio/AudioEngine.h>

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
    std::cout << "AudioBus Ducking Test" << std::endl;
    std::cout << "=====================" << std::endl;

    AudioEngine engine;
    AudioBus *music = engine.GetMusic();
    AudioBus *sfx   = engine.GetSFX();

    // 1. 默认状态
    CheckNear("默认 duck_scale == 1.0", music->GetDuckScale(), 1.0f);
    Check("默认 !IsDucked", !music->IsDucked());

    // 2. 立即 Duck（duration=0）
    music->Duck(0.2f, 0.0, 0.0);
    CheckNear("Duck(0.2) 立即 duck_scale == 0.2", music->GetDuckScale(), 0.2f);
    Check("IsDucked == true", music->IsDucked());
    CheckNear("有效增益 = gain(1.0) * duck(0.2)", music->GetEffectiveGain(), 0.2f);
    CheckNear("GetGain 仍为 1.0（自身增益不变）", music->GetGain(), 1.0f);

    // 3. 立即 Unduck
    music->Unduck(0.0, 0.0);
    CheckNear("Unduck 立即 duck_scale == 1.0", music->GetDuckScale(), 1.0f);
    Check("Unduck 后 !IsDucked", !music->IsDucked());
    CheckNear("Unduck 后有效增益 == 1.0", music->GetEffectiveGain(), 1.0f);

    // 4. 平滑过渡（1 秒内 1.0 -> 0.2）
    music->Duck(0.2f, 1.0, 0.0);
    music->Update(0.0);
    CheckNear("t=0 时仍为 1.0", music->GetDuckScale(), 1.0f);

    music->Update(0.5);
    const float mid = music->GetDuckScale();
    Check("t=0.5 时在 (0.2,1.0) 之间", mid > 0.2f && mid < 1.0f);

    music->Update(1.0);
    CheckNear("t=1.0 时完成 == 0.2", music->GetDuckScale(), 0.2f);

    music->Unduck(1.0, 0.0);    // 恢复过渡
    music->Update(1.0);
    CheckNear("恢复过渡完成 == 1.0", music->GetDuckScale(), 1.0f);

    // 5. Duck 传播到子树（master duck → music/sfx 有效增益同比例压低）
    engine.GetMaster()->Duck(0.5f, 0.0, 0.0);
    CheckNear("master duck 0.5 -> music effective 0.5", music->GetEffectiveGain(), 0.5f);
    CheckNear("master duck 0.5 -> sfx effective 0.5",   sfx->GetEffectiveGain(), 0.5f);

    // 6. Duck 与 gain 叠加
    sfx->SetGain(0.5f);
    CheckNear("sfx gain 0.5 × master duck 0.5 -> 0.25", sfx->GetEffectiveGain(), 0.25f);
    CheckNear("music 不受 sfx gain 影响 -> 0.5", music->GetEffectiveGain(), 0.5f);

    // 7. Duck 与 mute 组合
    engine.GetMaster()->Unduck(0.0, 0.0);
    sfx->SetGain(1.0f);
    sfx->SetMute(true);
    CheckNear("sfx mute -> effective 0", sfx->GetEffectiveGain(), 0.0f);
    sfx->SetMute(false);
    CheckNear("sfx 恢复 -> effective 1.0", sfx->GetEffectiveGain(), 1.0f);

    // 8. 递归 Update 不崩溃（master.Update 驱动整棵树）
    engine.Update(0.1);

    std::cout << std::endl;
    if(failed == 0)
    {
        std::cout << "全部通过" << std::endl;
        return 0;
    }

    std::cout << failed << " 项失败" << std::endl;
    return 1;
}
