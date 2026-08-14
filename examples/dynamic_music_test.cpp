// DynamicMusic Test
// 验证动态音乐分层：层/状态管理 + 状态切换 + crossfade 平滑过渡（纯逻辑，nullptr 播放器）
#include <iostream>
#include <hgl/audio/DynamicMusic.h>

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
    std::cout << "DynamicMusic Test" << std::endl;
    std::cout << "=================" << std::endl;

    DynamicMusic music;

    Check("初始 current_state == -1", music.GetCurrentState() == -1);
    Check("初始无层无状态", music.GetLayerCount() == 0 && music.GetStateCount() == 0);

    // 1. 添加层
    const int drum   = music.AddLayer(nullptr, 1.0f);
    const int melody = music.AddLayer(nullptr, 1.0f);
    Check("AddLayer 返回 0/1", drum == 0 && melody == 1);
    Check("2 层", music.GetLayerCount() == 2);
    Check("层播放器为 nullptr", music.GetLayerPlayer(0) == nullptr && music.GetLayerPlayer(1) == nullptr);

    // 2. 添加状态（探索：鼓满、旋律弱；战斗：都满）
    const float explore[] = {1.0f, 0.2f};
    const float combat [] = {1.0f, 1.0f};

    const int s_explore = music.AddState(OS_TEXT("explore"), explore, 2);
    const int s_combat  = music.AddState(OS_TEXT("combat"),  combat,  2);
    Check("AddState 返回 0/1", s_explore == 0 && s_combat == 1);
    Check("2 状态", music.GetStateCount() == 2);

    // 3. 立即切换（crossfade=0）
    Check("SetState(explore) 立即", music.SetState(OS_TEXT("explore"), 0.0, 0.0));
    Check("current_state == 0", music.GetCurrentState() == 0);
    CheckNear("层0 gain == 1.0", music.GetLayerGain(0), 1.0f);
    CheckNear("层1 gain == 0.2", music.GetLayerGain(1), 0.2f);

    // 4. crossfade 过渡到 combat（2 秒）
    Check("SetState(combat) 2 秒过渡", music.SetState(OS_TEXT("combat"), 0.0, 2.0));
    Check("current_state == 1", music.GetCurrentState() == 1);

    music.Update(0.0);
    CheckNear("t=0 层1 仍 0.2", music.GetLayerGain(1), 0.2f);

    music.Update(1.0);
    const float mid = music.GetLayerGain(1);
    Check("t=1.0 层1 在 (0.2,1.0)", mid > 0.2f && mid < 1.0f);

    music.Update(2.0);
    CheckNear("t=2.0 层1 == 1.0", music.GetLayerGain(1), 1.0f);

    CheckNear("层0 始终 1.0", music.GetLayerGain(0), 1.0f);

    // 5. 立即切回 explore（crossfade=0）
    Check("SetState(explore) 立即", music.SetState(s_explore, 0.0, 0.0));
    CheckNear("层1 立即回到 0.2", music.GetLayerGain(1), 0.2f);

    // 6. 非法状态
    Check("SetState 非法 index false", !music.SetState(99, 0.0, 0.0));
    Check("SetState 非法 name false",  !music.SetState(OS_TEXT("nonexistent"), 0.0, 0.0));
    Check("current_state 不变", music.GetCurrentState() == 0);

    // 7. 状态增益不足的层按 1.0 处理（添加第 3 层，状态只定义 2 个增益）
    music.AddLayer(nullptr, 0.8f);   // 层2，base_gain=0.8
    music.SetState(OS_TEXT("combat"), 0.0, 0.0);
    CheckNear("层2 增益缺省 -> base_gain 0.8", music.GetLayerGain(2), 0.8f);

    std::cout << std::endl;
    if(failed == 0)
    {
        std::cout << "全部通过" << std::endl;
        return 0;
    }

    std::cout << failed << " 项失败" << std::endl;
    return 1;
}
