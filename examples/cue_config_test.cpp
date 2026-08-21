// Cue Config Test (T2)
// 验证 Cue 配置扩展：RTPC 映射、轮播 sequence、复合 children、混音快照、TOML 解析
// 纯内存，无需设备
#include <iostream>
#include <cmath>
#include <hgl/audio/SoundEventManager.h>

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
    std::cout << "== Cue Config Test (T2: RTPC/sequence/children/snapshot) ==" << std::endl;

    // ---- 1. RTPC 线性映射 ----
    std::cout << "[1] RTPCConfig::Map" << std::endl;
    {
        RTPCConfig rpm;
        rpm.param = OS_TEXT("rpm");
        rpm.min = 0.0f;
        rpm.max = 8000.0f;
        rpm.target = RTPCTarget::Pitch;
        rpm.min_value = 0.5f;
        rpm.max_value = 2.0f;

        CheckNear("rpm=0 → pitch 0.5", rpm.Map(0.0f), 0.5f, 0.001f);
        CheckNear("rpm=4000 → pitch 1.25", rpm.Map(4000.0f), 1.25f, 0.001f);
        CheckNear("rpm=8000 → pitch 2.0", rpm.Map(8000.0f), 2.0f, 0.001f);
        CheckNear("rpm=16000（越界 clamp 到 2.0）", rpm.Map(16000.0f), 2.0f, 0.001f);
        CheckNear("rpm=-1000（越界 clamp 到 0.5）", rpm.Map(-1000.0f), 0.5f, 0.001f);

        // 字符串转换
        Check("RTPCTargetFromString(pitch)", RTPCTargetFromString("pitch")==RTPCTarget::Pitch);
        Check("RTPCTargetFromString(gain)",  RTPCTargetFromString("gain")==RTPCTarget::Gain);
        Check("RTPCTargetFromString(lowpass)", RTPCTargetFromString("lowpass")==RTPCTarget::Lowpass);
        Check("RTPCTargetFromString(pan)",   RTPCTargetFromString("pan")==RTPCTarget::Pan);
        Check("RTPCTargetToString(Pitch)",   strcmp(RTPCTargetToString(RTPCTarget::Pitch),"pitch")==0);
    }

    // ---- 2. sequence 轮播 ----
    std::cout << "[2] sequence 轮播" << std::endl;
    {
        SoundEventConfig cfg;
        cfg.sequence.Add(OS_TEXT("step_l.wav"));
        cfg.sequence.Add(OS_TEXT("step_r.wav"));

        CheckNear("第 0 次 → step_l", strcmp(cfg.SequenceFile(0)->c_str(),"step_l.wav")==0 ? 1.0f : 0.0f, 1.0f, 0.01f);
        CheckNear("第 1 次 → step_r", strcmp(cfg.SequenceFile(1)->c_str(),"step_r.wav")==0 ? 1.0f : 0.0f, 1.0f, 0.01f);
        CheckNear("第 2 次 → 回到 step_l（取模）", strcmp(cfg.SequenceFile(2)->c_str(),"step_l.wav")==0 ? 1.0f : 0.0f, 1.0f, 0.01f);
        CheckNear("第 7 次 → step_r（取模）", strcmp(cfg.SequenceFile(7)->c_str(),"step_r.wav")==0 ? 1.0f : 0.0f, 1.0f, 0.01f);

        // 无 sequence 返回 nullptr
        SoundEventConfig empty;
        Check("无 sequence → nullptr", empty.SequenceFile(0)==nullptr);
    }

    // ---- 3. 快照 CRUD ----
    std::cout << "[3] 快照 CRUD" << std::endl;
    {
        SoundEventManager manager;

        SnapshotConfig menu;
        menu.SetGain(AudioBusType::Music, -6.0f);
        menu.SetGain(AudioBusType::Ambient, -12.0f);

        Check("AddSnapshot(in_menu)", manager.AddSnapshot(OS_TEXT("in_menu"), menu));
        Check("ContainsSnapshot(in_menu)", manager.ContainsSnapshot(OS_TEXT("in_menu")));
        Check("GetSnapshotCount == 1", manager.GetSnapshotCount()==1);

        const SnapshotConfig *s = manager.GetSnapshot(OS_TEXT("in_menu"));
        Check("GetSnapshot 非空", s!=nullptr);
        if(s)
        {
            CheckNear("in_menu Music == -6dB", s->GetGain(AudioBusType::Music), -6.0f, 0.01f);
            CheckNear("in_menu Ambient == -12dB", s->GetGain(AudioBusType::Ambient), -12.0f, 0.01f);
            CheckNear("in_menu SFX == 0（未设置）", s->GetGain(AudioBusType::SFX), 0.0f, 0.01f);
        }

        Check("RemoveSnapshot(in_menu)", manager.RemoveSnapshot(OS_TEXT("in_menu")));
        Check("Remove 后不存在", !manager.ContainsSnapshot(OS_TEXT("in_menu")));
    }

    // ---- 4. TOML 解析扩展字段 ----
    std::cout << "[4] TOML 解析（sequence/children/rtpc/snapshot）" << std::endl;
    {
        SoundEventManager manager;

        Check("LoadFromTOML 成功", manager.LoadFromTOML("configs/sound_events.toml"));

        // sequence 事件
        const SoundEventConfig *step = manager.GetEvent(OS_TEXT("step_cycle"));
        Check("step_cycle 存在", step!=nullptr);
        if(step)
        {
            Check("step_cycle sequence 2 项", step->sequence.GetCount()==2);
            CheckNear("SequenceFile(0) == step_l",
                strcmp(step->SequenceFile(0)->c_str(),"wav_samples/step_l.wav")==0 ? 1.0f : 0.0f, 1.0f, 0.01f);
        }

        // children 事件
        const SoundEventConfig *combo = manager.GetEvent(OS_TEXT("combo_hit"));
        Check("combo_hit 存在", combo!=nullptr);
        if(combo)
        {
            Check("combo_hit children 2 项", combo->children.GetCount()==2);
            CheckNear("combo_hit priority == 4.0", combo->priority, 4.0f, 0.01f);
        }

        // RTPC 事件
        const SoundEventConfig *rpm = manager.GetEvent(OS_TEXT("engine_rpm"));
        Check("engine_rpm 存在", rpm!=nullptr);
        if(rpm)
        {
            Check("engine_rpm 有 2 条 rtpc", rpm->rtpc.size()==2);
            if(rpm->rtpc.size()==2)
            {
                const RTPCConfig &r0 = rpm->rtpc[0];
                CheckNear("rtpc[0] min == 0", r0.min, 0.0f, 0.01f);
                CheckNear("rtpc[0] max == 8000", r0.max, 8000.0f, 0.01f);
                Check("rtpc[0] target == Pitch", r0.target==RTPCTarget::Pitch);
                CheckNear("rtpc[0] Map(4000) == 1.25", r0.Map(4000.0f), 1.25f, 0.01f);

                const RTPCConfig &r1 = rpm->rtpc[1];
                Check("rtpc[1] target == Lowpass", r1.target==RTPCTarget::Lowpass);
                CheckNear("rtpc[1] Map(4000) == 4100", r1.Map(4000.0f), 4100.0f, 1.0f);
            }
        }

        // 快照解析
        const SnapshotConfig *menu = manager.GetSnapshot(OS_TEXT("in_menu"));
        Check("snapshot in_menu 存在", menu!=nullptr);
        if(menu)
        {
            CheckNear("in_menu Music == -6dB", menu->GetGain(AudioBusType::Music), -6.0f, 0.01f);
            CheckNear("in_menu SFX == -3dB", menu->GetGain(AudioBusType::SFX), -3.0f, 0.01f);
            CheckNear("in_menu Ambient == -12dB", menu->GetGain(AudioBusType::Ambient), -12.0f, 0.01f);
            CheckNear("in_menu UI == 0dB", menu->GetGain(AudioBusType::UI), 0.0f, 0.01f);
        }

        const SnapshotConfig *battle = manager.GetSnapshot(OS_TEXT("battle"));
        Check("snapshot battle 存在", battle!=nullptr);
        if(battle)
        {
            CheckNear("battle Music == -3dB", battle->GetGain(AudioBusType::Music), -3.0f, 0.01f);
            CheckNear("battle UI == -6dB", battle->GetGain(AudioBusType::UI), -6.0f, 0.01f);
        }
    }

    std::cout << "== 结果: " << (failed ? "FAILED" : "ALL PASSED") << " (" << failed << " failures) ==" << std::endl;
    return failed ? 1 : 0;
}
