// Time Effects Test
// 验证 DelayLine / Echo / Chorus（时域效果，P4）（纯数学，无需 OpenAL）
#include <iostream>
#include <cmath>
#include <hgl/audio/TimeEffects.h>

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
    std::cout << "Time Effects Test" << std::endl;
    std::cout << "=================" << std::endl;

    // ===== 1. DelayLine 基本读写 =====
    {
        DelayLine line;
        line.Init(100);
        line.Write(1.0f);
        line.Write(2.0f);
        line.Write(3.0f);

        CheckNear("Read(0) == 3.0（刚写）", line.Read(0.0f), 3.0f, 0.0001f);
        CheckNear("Read(1) == 2.0", line.Read(1.0f), 2.0f, 0.0001f);
        CheckNear("Read(2) == 1.0", line.Read(2.0f), 1.0f, 0.0001f);
    }

    // ===== 2. DelayLine 环形回绕 =====
    {
        DelayLine line;
        line.Init(4);
        line.Write(1.0f);   // buffer[0]
        line.Write(2.0f);   // buffer[1]
        line.Write(3.0f);   // buffer[2]
        line.Write(4.0f);   // buffer[3]，write_index 回绕

        CheckNear("回绕后 Read(0) == 4.0", line.Read(0.0f), 4.0f, 0.0001f);
        CheckNear("回绕后 Read(3) == 1.0", line.Read(3.0f), 1.0f, 0.0001f);

        line.Write(5.0f);   // 覆盖 buffer[0]
        CheckNear("覆盖后 Read(0) == 5.0", line.Read(0.0f), 5.0f, 0.0001f);
        CheckNear("覆盖后 Read(3) == 2.0", line.Read(3.0f), 2.0f, 0.0001f);
    }

    // ===== 3. DelayLine 插值读取 =====
    {
        DelayLine line;
        line.Init(100);
        line.Write(1.0f);
        line.Write(2.0f);
        line.Write(3.0f);

        // delay 0.5 = 在刚写(3.0) 和 前一个(2.0) 之间 = 2.5
        CheckNear("ReadInterpolated(0.5) == 2.5", line.ReadInterpolated(0.5f), 2.5f, 0.0001f);
        CheckNear("ReadInterpolated(0.0) == 3.0", line.ReadInterpolated(0.0f), 3.0f, 0.0001f);
        CheckNear("ReadInterpolated(1.0) == 2.0", line.ReadInterpolated(1.0f), 2.0f, 0.0001f);
    }

    // ===== 4. Echo 纯延迟（feedback=0, mix=1 全湿）=====
    {
        Echo echo;
        echo.Init(48000.0f, 0.001f, 0.0f, 1.0f);   // 延迟 48 样本

        bool correct = true;
        for(int i = 0; i < 100; i++)
        {
            const float out = echo.Process(i == 0 ? 1.0f : 0.0f);   // i=0 输入脉冲
            if(i == 48)
            {
                if(std::fabs(out - 1.0f) > 0.0001f) correct = false;
            }
            else if(std::fabs(out) > 0.0001f)
                correct = false;
        }
        Check("Echo 纯延迟：脉冲在 48 样本后输出", correct);
    }

    // ===== 5. Echo 反馈（feedback=0.5, mix=0.5）=====
    {
        Echo echo;
        echo.Init(48000.0f, 0.001f, 0.5f, 0.5f);   // 延迟 48，反馈 0.5

        // 输入脉冲，混干湿：i=0 输出 0.5（干），i=48 输出 0.5（湿），i=96 输出 0.25（二次回声）
        const float out0 = echo.Process(1.0f);
        CheckNear("反馈 i=0 输出 0.5（干信号）", out0, 0.5f, 0.0001f);

        for(int i = 1; i < 48; i++) echo.Process(0.0f);   // i=1..47

        const float out48 = echo.Process(0.0f);   // i=48
        CheckNear("反馈 i=48 输出 0.5（一次回声）", out48, 0.5f, 0.0001f);

        for(int i = 49; i < 96; i++) echo.Process(0.0f);   // i=49..95

        const float out96 = echo.Process(0.0f);   // i=96
        CheckNear("反馈 i=96 输出 0.25（二次回声）", out96, 0.25f, 0.0001f);
    }

    // ===== 6. Echo mix=0 全干 =====
    {
        Echo echo;
        echo.Init(48000.0f, 0.001f, 0.5f, 0.0f);
        CheckNear("mix=0 输出 == 输入", echo.Process(0.7f), 0.7f, 0.0001f);
    }

    // ===== 7. Chorus 基本：mix=0 全干，Reset 后重来 =====
    {
        Chorus c = Chorus::CreateChorus(48000.0f);

        // mix=0 直通验证（用自定义参数）
        Chorus dry;
        dry.Init(48000.0f, 0.020f, 0.5f, 0.005f, 0.0f, 0.0f);
        CheckNear("Chorus mix=0 输出 == 输入", dry.Process(0.6f), 0.6f, 0.0001f);

        // 有湿信号时输出非零（正弦输入）
        bool non_zero = false;
        Chorus wet = Chorus::CreateChorus(48000.0f);
        for(int i = 0; i < 1000; i++)
        {
            const float x = 0.5f * std::sin(2.0f * 3.14159265f * 440.0f * i / 48000.0f);
            if(std::fabs(wet.Process(x)) > 0.01f) non_zero = true;
        }
        Check("Chorus 有湿信号输出非零", non_zero);

        // Reset 后状态归零
        wet.Reset();
        CheckNear("Chorus Reset 后直通（首个样本干信号）", wet.Process(0.5f), 0.25f, 0.0001f);   // mix=0.5 → 干 0.25 + 湿 0
    }

    // ===== 8. Flanger 工厂（不崩溃 + 输出非零）=====
    {
        Chorus fl = Chorus::CreateFlanger(48000.0f);
        bool non_zero = false;
        for(int i = 0; i < 2000; i++)
        {
            const float x = 0.3f * std::sin(2.0f * 3.14159265f * 220.0f * i / 48000.0f);
            if(std::fabs(fl.Process(x)) > 0.01f) non_zero = true;
        }
        Check("Flanger 输出非零", non_zero);
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
