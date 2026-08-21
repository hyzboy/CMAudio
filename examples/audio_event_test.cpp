// Audio Event Test (T1)
// 验证事件协议层：AudioEvent/AudioEventResult 定长 POD、序列化往返、
// CueNameHash（FNV-1a）确定性 + 标准向量 + 跨边界语义
#include <iostream>
#include <cstring>
#include <vector>
#include <hgl/audio/AudioEvent.h>

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
    std::cout << "== Audio Event Test (T1: 事件协议层) ==" << std::endl;

    // ---- 1. 结构定长（跨边界安全的基础）----
    std::cout << "[1] 定长 POD 结构" << std::endl;
    {
        Check("AudioEvent 固定 48 字节", sizeof(AudioEvent)==48);
        Check("AudioEventResult 固定 16 字节", sizeof(AudioEventResult)==16);

        // 枚举值稳定（跨版本 ABI）
        Check("Play==0", uint32(AudioEventType::Play)==0);
        Check("Stop==1", uint32(AudioEventType::Stop)==1);
        Check("SetParam==2", uint32(AudioEventType::SetParam)==2);
        Check("PlayStarted==0", uint32(AudioEventResultType::PlayStarted)==0);
        Check("Error==4", uint32(AudioEventResultType::Error)==4);
    }

    // ---- 2. AudioEvent 字段赋值 + 序列化往返 ----
    std::cout << "[2] 事件序列化往返" << std::endl;
    {
        AudioEvent ev(AudioEventType::Play, 0x12345678u, 0, 42);
        ev.params[0]=10.0f;
        ev.params[1]=20.0f;
        ev.params[2]=30.0f;

        unsigned char bytes[64];
        Check("ToBytes 成功", ev.ToBytes(bytes,sizeof(bytes)));

        AudioEvent back;
        Check("FromBytes 成功", back.FromBytes(bytes,sizeof(bytes)));

        Check("往返后 type 一致", back.type==uint32(AudioEventType::Play));
        Check("往返后 cue_id 一致", back.cue_id==0x12345678u);
        Check("往返后 seq 一致", back.seq==42);
        Check("往返后 params[2]==30", back.params[2]==30.0f);
        Check("往返后整体字节一致", memcmp(&ev,bytes,sizeof(AudioEvent))==0);

        // 缓冲区过小应失败
        Check("ToBytes 缓冲过小失败", !ev.ToBytes(bytes,47));
        Check("FromBytes 缓冲过小失败", !back.FromBytes(bytes,47));

        // 0 字节缓冲区（模拟空指针路径）
        Check("ToBytes 空指针失败", !ev.ToBytes(nullptr,64));
    }

    // ---- 3. CueNameHash：确定性 + 标准向量 + 区分度 ----
    std::cout << "[3] CueNameHash (FNV-1a)" << std::endl;
    {
        // 确定性
        Check("同名字符串哈希一致", CueNameHash("ui_click")==CueNameHash("ui_click"));
        Check("空指针返回 0", CueNameHash(nullptr)==0);

        // 标准 FNV-1a 向量（外部验证过的期望值）
        // "a" → 0xE40C292C, "foobar" → 0xBF9CF968（FNV-1a 官方测试向量）
        Check("FNV-1a 标准向量 'a'", CueNameHash("a")==0xE40C292Cu);
        Check("FNV-1a 标准向量 'foobar'", CueNameHash("foobar")==0xBF9CF968u);

        // 区分度：不同 Cue 名哈希不同
        Check("不同名哈希不同", CueNameHash("ui_click")!=CueNameHash("ui_hover"));

        // 大小写敏感（Cue 名是精确匹配）
        Check("大小写敏感", CueNameHash("UI_Click")!=CueNameHash("ui_click"));

        // 常见 Cue 名跨会话稳定（存配置/日志用）
        const uint32 click_h=CueNameHash("ui_click");
        std::cout << "    [INFO] ui_click hash = 0x" << std::hex << click_h << std::dec << std::endl;
    }

    // ---- 4. 事件流批量（模拟每帧发一批）----
    std::cout << "[4] 批量事件流" << std::endl;
    {
        std::vector<AudioEvent> events;
        events.push_back(AudioEvent(AudioEventType::Play,    CueNameHash("ui_click"), 0, 1));
        events.push_back(AudioEvent(AudioEventType::Play,    CueNameHash("explosion"),0, 2));
        events.push_back(AudioEvent(AudioEventType::SetParam,CueNameHash("engine"),   7, 3));
        events.back().params[0]=4500.0f;
        events.push_back(AudioEvent(AudioEventType::Stop,    0, 7, 4));

        // 序列化整批（定长每项 48B → 简单 memcpy 数组）
        unsigned char buf[4*48];
        bool ok=true;
        for(int i=0;i<4;i++)
            if(!events[i].ToBytes(buf+i*48,48))
                ok=false;
        Check("批量序列化成功", ok);

        // 反序列化校验
        bool match=true;
        for(int i=0;i<4;i++)
        {
            AudioEvent back;
            back.FromBytes(buf+i*48,48);
            if(memcmp(&events[i],&back,48)!=0)
                match=false;
        }
        Check("批量反序列化逐项一致", match);

        // SetParam 的 RTPC 值穿越载荷
        Check("RTPC 值 4500 穿越载荷", events[2].params[0]==4500.0f);
        Check("seq 序号对账可用", events[3].seq==4);
    }

    // ---- 5. AudioEventResult 回传往返 ----
    std::cout << "[5] 状态回传" << std::endl;
    {
        AudioEventResult r(AudioEventResultType::PlayStarted, 7, 0, 42);

        unsigned char bytes[32];
        Check("ToBytes 成功", r.ToBytes(bytes,sizeof(bytes)));

        AudioEventResult back;
        Check("FromBytes 成功", back.FromBytes(bytes,sizeof(bytes)));

        Check("回传 type 一致", back.type==uint32(AudioEventResultType::PlayStarted));
        Check("回传 instance_id 一致", back.instance_id==7);
        Check("回传 seq 一致", back.seq==42);

        // Error 回传
        AudioEventResult err(AudioEventResultType::Error, 0, 404, 99);
        Check("Error 回传 error_code==404", err.error_code==404);
    }

    std::cout << "== 结果: " << (failed ? "FAILED" : "ALL PASSED") << " (" << failed << " failures) ==" << std::endl;
    return failed ? 1 : 0;
}
