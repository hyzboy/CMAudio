// Audio Client Test (T6: DLL 模式 C API)
// 客户端视角：只 include AudioClient.h，链接 CMP.AudioClient.dll
// 验证：生命周期/Cue 注册/事件发送/状态回传/总线/快照/Stop
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <hgl/audio/AudioClient.h>

static int failed = 0;

static void Check(const char *name, int cond)
{
    printf("  [%s] %s\n", cond ? "PASS" : "FAIL", name);
    if(!cond) ++failed;
}

// 生成 0.5 秒 440Hz 单声道 16bit 正弦 WAV（C 实现，纯客户端无 C++ 依赖）
static int WriteTestWav(const char *path)
{
    const int sample_rate = 16000;
    const int seconds = 1;
    const int samples = sample_rate * seconds;

    FILE *f = fopen(path, "wb");
    if(!f) return 0;

    /* RIFF 头 */
    const int data_size = samples * 2;
    const int riff_size = 36 + data_size;

    fwrite("RIFF", 1, 4, f);
    fwrite(&riff_size, 4, 1, f);
    fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f);

    const int fmt_size = 16;
    const short audio_format = 1;
    const short channels = 1;
    const int byte_rate = sample_rate * 2;
    const short block_align = 2;
    const short bits = 16;

    fwrite(&fmt_size, 4, 1, f);
    fwrite(&audio_format, 2, 1, f);
    fwrite(&channels, 2, 1, f);
    fwrite(&sample_rate, 4, 1, f);
    fwrite(&byte_rate, 4, 1, f);
    fwrite(&block_align, 2, 1, f);
    fwrite(&bits, 2, 1, f);

    fwrite("data", 1, 4, f);
    fwrite(&data_size, 4, 1, f);

    for(int i = 0; i < samples; i++)
    {
        const short v = (short)(sin(2.0 * 3.14159265358979 * 440.0 * i / sample_rate) * 32767 * 0.5);
        fwrite(&v, 2, 1, f);
    }

    fclose(f);
    return 1;
}

int main()
{
    printf("== Audio Client Test (T6: DLL 模式 C API) ==\n");

    if(!WriteTestWav("test_tone.wav"))
    {
        printf("  [FAIL] 无法生成测试音频\n");
        return 1;
    }

    // ---- 1. 生命周期 + Cue 注册 ----
    printf("[1] 生命周期与 Cue 注册\n");

    AudioClient *c = AudioClient_Create(0);
    Check("Create 成功", c != NULL);

    Check("AddCue 成功", AudioClient_AddCue(c, "test_tone", "test_tone.wav"));
    Check("未知 Cue 添加失败(空名)", !AudioClient_AddCue(c, "", "test_tone.wav"));

    Check("Start 成功", AudioClient_Start(c));

    // ---- 2. Play → PlayStarted 回传 ----
    printf("[2] Play → PlayStarted\n");

    uint32_t seq = 1;
    Check("Play 发送成功", AudioClient_Play(c, "test_tone", seq, NULL));
    Check("WaitIdle", AudioClient_WaitIdle(c, 3000));

    AudioClientResult r;
    int started = 0;
    while(AudioClient_PollResult(c, &r))
    {
        if(r.type == 0 && r.error_code == 0)      // PlayStarted
            ++started;
    }

    Check("收到 PlayStarted", started == 1);
    Check("活跃实例 1", AudioClient_GetActiveInstanceCount(c) == 1);
    Check("processed >= 1", AudioClient_GetProcessedCount(c) >= 1);

    // ---- 3. 未知 Cue → Error ----
    printf("[3] 未知 Cue → Error\n");

    seq++;
    Check("Play 未知 Cue 发送成功", AudioClient_Play(c, "nonexistent", seq, NULL));
    Check("WaitIdle", AudioClient_WaitIdle(c, 3000));

    int got_error = 0;
    while(AudioClient_PollResult(c, &r))
    {
        if(r.type == 4)                            // Error
            got_error = 1;
    }

    Check("收到 Error", got_error);

    // ---- 4. 总线 + 快照 ----
    printf("[4] 总线与快照\n");

    seq++;
    Check("SetBusVolume 发送成功", AudioClient_SetBusVolume(c, AudioClientBus_SFX, 0.5f, seq));
    Check("WaitIdle", AudioClient_WaitIdle(c, 3000));

    seq++;
    Check("Snapshot 发送成功", AudioClient_Snapshot(c, "menu", seq));
    Check("WaitIdle", AudioClient_WaitIdle(c, 3000));

    // 快照 "menu" 未注册 → 无效果（不崩即可，通过）
    Check("快照无崩溃", 1);

    // ---- 5. Stop → Stopped ----
    printf("[5] Stop → Stopped\n");

    seq++;
    Check("Stop 发送成功", AudioClient_StopInstance(c, 1, seq));
    Check("WaitIdle", AudioClient_WaitIdle(c, 3000));

    int stopped = 0;
    while(AudioClient_PollResult(c, &r))
    {
        if(r.type == 2)                            // Stopped
            ++stopped;
    }

    Check("收到 Stopped", stopped >= 1);
    Check("实例已清理", AudioClient_GetActiveInstanceCount(c) == 0);

    // ---- 6. 销毁 ----
    printf("[6] 销毁\n");

    AudioClient_Stop(c);
    AudioClient_Destroy(c);
    Check("销毁完成", 1);

    printf("== 结果: %s (%d failures) ==\n", failed ? "FAILED" : "ALL PASSED", failed);
    return failed ? 1 : 0;
}
