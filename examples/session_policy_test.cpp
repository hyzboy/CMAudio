// AudioSessionPolicy Test
// 验证移动平台音频会话策略抽象（桌面实现，纯逻辑，无需 OpenAL）
#include <iostream>
#include <hgl/audio/AudioSessionPolicy.h>

using namespace hgl::audio;

static int failed = 0;

static void Check(const char *name, bool cond)
{
    std::cout << (cond ? "  [PASS] " : "  [FAIL] ") << name << std::endl;
    if(!cond) ++failed;
}

int main()
{
    std::cout << "AudioSessionPolicy Test" << std::endl;
    std::cout << "=======================" << std::endl;

    // 1. 桌面默认实现状态机
    DesktopSessionPolicy policy;
    Check("默认 HasFocus", policy.GetFocusState() == AudioFocusState::HasFocus);
    Check("桌面非静音", !policy.IsSilenced());
    Check("Initialize 成功", policy.Initialize());

    Check("RequestFocus 成功", policy.RequestFocus());
    Check("RequestFocus 后 HasFocus", policy.GetFocusState() == AudioFocusState::HasFocus);

    policy.AbandonFocus();
    Check("AbandonFocus 后 Lost", policy.GetFocusState() == AudioFocusState::Lost);

    Check("重新 RequestFocus 恢复 HasFocus", policy.RequestFocus());
    Check("恢复后 HasFocus", policy.GetFocusState() == AudioFocusState::HasFocus);

    policy.Update();   // 轮询不崩溃

    // 2. 工厂创建
    AudioSessionPolicy *p = CreateSessionPolicy();
    Check("工厂创建非空", p != nullptr);
    if(p)
    {
        Check("工厂策略 Initialize", p->Initialize());
        Check("工厂策略 RequestFocus", p->RequestFocus());
        Check("工厂策略 HasFocus", p->GetFocusState() == AudioFocusState::HasFocus);
        Check("工厂策略非静音", !p->IsSilenced());
        p->Update();
        delete p;
    }

    std::cout << std::endl;
    if(failed == 0)
    {
        std::cout << "全部通过" << std::endl;
        return 0;
    }

    std::cout << failed << " 项失败" << std::endl;
    return 1;
}
