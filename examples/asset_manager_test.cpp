// AudioAssetManager Test
// 验证资源缓存去重、引用计数、全量/流式决策（纯逻辑：用 Register 注入空 buffer，绕开 OpenAL 设备与文件加载）
#include <iostream>
#include <hgl/platform/Platform.h>
#include <hgl/audio/AudioAssetManager.h>
#include <hgl/audio/AudioBuffer.h>

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
    std::cout << "AudioAssetManager Test" << std::endl;
    std::cout << "======================" << std::endl;

    // 1. 全量 vs 流式自动决策（文件大小启发式，默认阈值 1MB）
    Check("0 字节 -> Full",           SuggestAudioLoadMode(0) == AudioLoadMode::Full);
    Check("512KB -> Full",            SuggestAudioLoadMode(512*1024) == AudioLoadMode::Full);
    Check("1MB (阈值内) -> Full",     SuggestAudioLoadMode(1024*1024) == AudioLoadMode::Full);
    Check("1MB+1 -> Stream",          SuggestAudioLoadMode(1024*1024+1) == AudioLoadMode::Stream);
    Check("16MB -> Stream",           SuggestAudioLoadMode(16*1024*1024) == AudioLoadMode::Stream);
    Check("负值(未知大小) -> Stream", SuggestAudioLoadMode(-1) == AudioLoadMode::Stream);

    // 2. 登记 / 查找 / 计数
    AudioAssetManager am;

    AudioBuffer *b1 = new AudioBuffer();   // 空 buffer（filename=nullptr），不碰 OpenAL
    AudioBuffer *b2 = new AudioBuffer();

    Check("初始 GetCount == 0",       am.GetCount() == 0);
    Check("Register(a,b1) 成功",      am.Register(OS_TEXT("a"), b1));
    Check("Register(b,b2) 成功",      am.Register(OS_TEXT("b"), b2));
    Check("Register 重名失败",        !am.Register(OS_TEXT("a"), b2));
    Check("GetCount == 2",            am.GetCount() == 2);
    Check("Contains(a) == true",      am.Contains(OS_TEXT("a")));
    Check("Contains(c) == false",     !am.Contains(OS_TEXT("c")));
    Check("Find(a) == b1",            am.Find(OS_TEXT("a")) == b1);
    Check("Find(c) == nullptr",       am.Find(OS_TEXT("c")) == nullptr);
    Check("Register 后 ref_count == 1", b1->GetRefCount() == 1);

    // 3. Acquire 命中缓存（去重：返回同一指针 + 引用计数递增）
    AudioBuffer *r1 = am.Acquire(OS_TEXT("a"));
    Check("Acquire(a) 命中 == b1",    r1 == b1);
    Check("命中后 ref_count == 2",    b1->GetRefCount() == 2);

    AudioBuffer *r2 = am.Acquire(OS_TEXT("a"));
    Check("再次 Acquire(a) == b1",    r2 == b1);
    Check("ref_count == 3",           b1->GetRefCount() == 3);

    // 4. Release 引用计数递减，归零才卸载
    am.Release(r1);
    Check("Release 一次 ref_count == 2", b1->GetRefCount() == 2);

    am.Release(r2);
    Check("Release 两次 ref_count == 1", b1->GetRefCount() == 1);
    Check("ref==1 时仍在缓存",        am.Contains(OS_TEXT("a")));

    am.Release(b1);                         // 归零 -> 卸载并 delete
    Check("归零后 !Contains(a)",      !am.Contains(OS_TEXT("a")));
    Check("GetCount == 1 (只剩 b)",   am.GetCount() == 1);

    // 5. Release(name) 形式
    am.Release(OS_TEXT("b"));
    Check("Release(b) 后 !Contains(b)", !am.Contains(OS_TEXT("b")));
    Check("GetCount == 0",            am.GetCount() == 0);

    // 6. Clear 强制清空
    AudioBuffer *b3 = new AudioBuffer();
    am.Register(OS_TEXT("c"), b3);
    Check("GetCount == 1",            am.GetCount() == 1);

    am.Clear();
    Check("Clear 后 GetCount == 0",   am.GetCount() == 0);

    // 7. 边界：释放不存在的键 / 空指针，不应崩溃
    am.Release(OS_TEXT("nonexistent"));
    am.Release(static_cast<AudioBuffer *>(nullptr));
    Check("边界释放不崩溃",            true);

    // 8. 异步 API 边界（无 OpenAL 设备/文件时的纯逻辑行为）
    Check("AcquireAsync 无法识别类型 -> false", !am.AcquireAsync(OS_TEXT("nonexistent.xyz")));
    Check("异步未启动 -> GetPendingCount == 0", am.GetPendingCount() == 0);
    Check("异步未启动 -> !IsLoading()",         !am.IsLoading());
    Check("异步未启动 -> Update() == 0",        am.Update() == 0);

    AudioBuffer *b4 = new AudioBuffer();
    am.Register(OS_TEXT("async_hit"), b4);
    Check("AcquireAsync 缓存命中 -> true",        am.AcquireAsync(OS_TEXT("async_hit")));
    Check("命中不启动线程 -> GetPendingCount == 0", am.GetPendingCount() == 0);
    am.Clear();

    std::cout << std::endl;
    if(failed == 0)
    {
        std::cout << "全部通过" << std::endl;
        return 0;
    }

    std::cout << failed << " 项失败" << std::endl;
    return 1;
}
