// SoundEvent Test
// 验证数据驱动层：总线分组转换、事件增删查、随机化、TOML 加载
#include <iostream>
#include <cstring>
#include <hgl/platform/Platform.h>
#include <hgl/audio/SoundEventManager.h>

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
    std::cout << "SoundEvent Test" << std::endl;
    std::cout << "===============" << std::endl;

    // 1. 总线分组转换
    Check("FromString(Master) == Master",   AudioBusTypeFromString("Master") == AudioBusType::Master);
    Check("FromString(Music) == Music",     AudioBusTypeFromString("Music") == AudioBusType::Music);
    Check("FromString(SFX) == SFX",         AudioBusTypeFromString("SFX") == AudioBusType::SFX);
    Check("FromString(Ambient) == Ambient", AudioBusTypeFromString("Ambient") == AudioBusType::Ambient);
    Check("FromString(UI) == UI",           AudioBusTypeFromString("UI") == AudioBusType::UI);
    Check("FromString(未知) -> SFX",        AudioBusTypeFromString("unknown") == AudioBusType::SFX);
    Check("ToString(SFX)", strcmp(AudioBusTypeToString(AudioBusType::SFX), "SFX") == 0);

    // 2. 手动添加/查询事件
    SoundEventManager manager;
    Check("初始 GetCount == 0", manager.GetCount() == 0);

    SoundEventConfig shoot;
    shoot.files.Add(OS_TEXT("shoot.wav"));
    shoot.min_gain = 0.8f;
    shoot.max_gain = 1.0f;
    shoot.min_pitch = 0.9f;
    shoot.max_pitch = 1.1f;
    shoot.bus_type = AudioBusType::SFX;

    Check("AddEvent(shoot)", manager.AddEvent(OS_TEXT("shoot"), shoot));
    Check("Contains(shoot)",  manager.Contains(OS_TEXT("shoot")));
    Check("GetCount == 1",    manager.GetCount() == 1);
    Check("GetEvent 未命中 == nullptr", manager.GetEvent(OS_TEXT("nonexistent")) == nullptr);

    const SoundEventConfig *ev = manager.GetEvent(OS_TEXT("shoot"));
    Check("GetEvent(shoot) 非空", ev != nullptr);

    // 3. 随机化在范围内
    bool in_range = true;
    if(ev)
    {
        for(int i=0;i<200;i++)
        {
            const float g = ev->RandomGain();
            const float p = ev->RandomPitch();
            if(g < 0.8f || g > 1.0f || p < 0.9f || p > 1.1f) { in_range = false; break; }
        }
    }
    Check("RandomGain/RandomPitch 在 [min,max] 内", in_range);
    Check("RandomFile 非空", ev && ev->RandomFile() != nullptr);

    // 4. 多文件变体随机覆盖
    SoundEventConfig multi;
    multi.files.Add(OS_TEXT("a.wav"));
    multi.files.Add(OS_TEXT("b.wav"));
    multi.files.Add(OS_TEXT("c.wav"));
    manager.AddEvent(OS_TEXT("multi"), multi);

    const SoundEventConfig *mev = manager.GetEvent(OS_TEXT("multi"));
    bool saw_a=false, saw_b=false, saw_c=false;
    for(int i=0;i<300;i++)
    {
        const OSString *f = mev?mev->RandomFile():nullptr;
        if(!f) { saw_a=saw_b=saw_c=false; break; }
        if(hgl::strcmp(f->c_str(), OS_TEXT("a.wav"))==0) saw_a=true;
        if(hgl::strcmp(f->c_str(), OS_TEXT("b.wav"))==0) saw_b=true;
        if(hgl::strcmp(f->c_str(), OS_TEXT("c.wav"))==0) saw_c=true;
    }
    Check("多文件随机覆盖 a/b/c", saw_a && saw_b && saw_c);

    // 5. 删除事件
    Check("RemoveEvent(multi)", manager.RemoveEvent(OS_TEXT("multi")));
    Check("!Contains(multi)",   !manager.Contains(OS_TEXT("multi")));

    // 6. TOML 加载
    manager.Clear();
    Check("Clear 后 GetCount == 0", manager.GetCount() == 0);

    Check("LoadFromTOML 成功", manager.LoadFromTOML("configs/sound_events.toml"));
    Check("加载后 GetCount == 8", manager.GetCount() == 8);

    const SoundEventConfig *boom = manager.GetEvent(OS_TEXT("explosion"));
    Check("explosion 存在", boom != nullptr);
    if(boom)
    {
        Check("explosion 有 2 个文件变体",  boom->files.GetCount() == 2);
        Check("explosion priority == 2.0",  boom->priority == 2.0f);
        Check("explosion max_distance == 500", boom->max_distance == 500.0f);
        Check("explosion bus == SFX",       boom->bus_type == AudioBusType::SFX);
        Check("explosion 未循环",           !boom->loop);
    }

    const SoundEventConfig *bgm = manager.GetEvent(OS_TEXT("bgm"));
    Check("bgm 存在且 loop",   bgm && bgm->loop);
    Check("bgm bus == Music",  bgm && bgm->bus_type == AudioBusType::Music);

    const SoundEventConfig *ui = manager.GetEvent(OS_TEXT("ui_click"));
    Check("ui_click bus == UI", ui && ui->bus_type == AudioBusType::UI);
    Check("ui_click priority == 3.0", ui && ui->priority == 3.0f);

    std::cout << std::endl;
    if(failed == 0)
    {
        std::cout << "全部通过" << std::endl;
        return 0;
    }

    std::cout << failed << " 项失败" << std::endl;
    return 1;
}
