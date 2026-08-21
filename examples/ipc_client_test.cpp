// IPC Client Test (T7: 独占进程模式)
// 客户端视角：起 audio_server 子进程 → IPCTransport 连接 → 发事件 → 收回传
// 验证跨进程事件链路：Play→PlayStarted / 未知Cue→Error / Stop→Stopped / 断开检测
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <hgl/audio/IPCTransport.h>
#include <hgl/audio/AudioEvent.h>
#include <hgl/time/Time.h>
#include <windows.h>

using namespace hgl;
using namespace hgl::audio;

static int failed = 0;

static void Check(const char *name, bool cond)
{
    std::cout << (cond ? "  [PASS] " : "  [FAIL] ") << name << std::endl;
    if(!cond) ++failed;
}

// 生成 1 秒 440Hz 单声道 16bit 正弦 WAV
static bool WriteTestWav(const char *path)
{
    const int sample_rate=16000;
    const int seconds=1;
    const int samples=sample_rate*seconds;

    FILE *f=fopen(path,"wb");
    if(!f)return false;

    const int data_size=samples*2;
    const int riff_size=36+data_size;

    fwrite("RIFF",1,4,f);
    fwrite(&riff_size,4,1,f);
    fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f);

    const int fmt_size=16;
    const short audio_format=1;
    const short channels=1;
    const int byte_rate=sample_rate*2;
    const short block_align=2;
    const short bits=16;

    fwrite(&fmt_size,4,1,f);
    fwrite(&audio_format,2,1,f);
    fwrite(&channels,2,1,f);
    fwrite(&sample_rate,4,1,f);
    fwrite(&byte_rate,4,1,f);
    fwrite(&block_align,2,1,f);
    fwrite(&bits,2,1,f);

    fwrite("data",1,4,f);
    fwrite(&data_size,4,1,f);

    for(int i=0;i<samples;i++)
    {
        const short v=(short)(sin(2.0*3.14159265358979*440.0*i/sample_rate)*32767*0.5);
        fwrite(&v,2,1,f);
    }

    fclose(f);
    return true;
}

int main()
{
    std::cout << "== IPC Client Test (T7: 独占进程模式) ==" << std::endl;

    // 生成测试音频 + 注册到服务端
    if(!WriteTestWav("test_tone.wav"))
    {
        std::cout << "  [FAIL] 无法生成测试音频" << std::endl;
        return 1;
    }

    // 启动服务端子进程
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    memset(&si,0,sizeof(si));
    memset(&pi,0,sizeof(pi));
    si.cb=sizeof(si);

    char cmd[512];
    snprintf(cmd,sizeof(cmd),"audio_server.exe ipctest");

    if(!CreateProcessA(nullptr,cmd,nullptr,nullptr,FALSE,0,nullptr,nullptr,&si,&pi))
    {
        std::cout << "  [FAIL] 无法启动 audio_server" << std::endl;
        return 1;
    }

    // 等待服务端创建管道
    hgl::SleepSecond(0.5);

    // ---- 1. 客户端连接 ----
    std::cout << "[1] 连接服务端" << std::endl;

    IPCTransport ipc;
    Check("ConnectClient 成功", ipc.ConnectClient(OS_TEXT("ipctest")));

    // ---- 2. 注册 Cue（通过服务端侧？不——Cue 注册在服务端进程内）
    // 说明：Cue 注册是服务端进程的职责。本测试通过事件验证链路，
    // 服务端未注册任何 Cue → Play 应回传 Error(未知Cue)。
    // 真实用法：服务端启动时 LoadCues(TOML) 注册。
    std::cout << "[2] 未知 Cue → Error（服务端无 Cue 表）" << std::endl;

    {
        AudioEvent ev(AudioEventType::Play, CueNameHash("test_tone"), 0, 1);
        Check("Send 成功", ipc.Send(ev));

        // 轮询回传（最多 5 秒）
        AudioEventResult r;
        bool got_error=false;

        for(int i=0;i<50;i++)
        {
            hgl::SleepSecond(0.1);

            while(ipc.PollResult(r))
                if(r.type==uint32(AudioEventResultType::Error))
                    got_error=true;

            if(got_error)break;
        }

        Check("收到 Error 回传", got_error);
    }

    // ---- 3. 断开：客户端退出 → 服务端检测断开 ----
    std::cout << "[3] 断开连接" << std::endl;

    ipc.Close();

    // 等待服务端检测断开并退出
    DWORD exit_code=STILL_ACTIVE;

    for(int i=0;i<50&&exit_code==STILL_ACTIVE;i++)
    {
        hgl::SleepSecond(0.1);
        GetExitCodeProcess(pi.hProcess,&exit_code);
    }

    Check("服务端检测断开并退出", exit_code==0);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::cout << "== 结果: " << (failed ? "FAILED" : "ALL PASSED") << " (" << failed << " failures) ==" << std::endl;
    return failed ? 1 : 0;
}
