// Compressor Test
// 验证动态范围压缩器：压缩比 / 直通 / makeup / limiter / attack-release 平滑（纯数学，无需 OpenAL）
#include <iostream>
#include <cmath>
#include <vector>
#include <hgl/audio/Compressor.h>
#include <hgl/audio/AudioAnalysis.h>

using namespace hgl::audio;

static const float PI = 3.14159265358979323846f;

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

// 生成方波（|x| 恒定 = amp，便于精确测量稳态压缩增益）
static void GenSquare(std::vector<float> &out, float amp, int count)
{
    out.resize(count);
    for(int i = 0; i < count; i++)
        out[i] = (i % 2 == 0) ? amp : -amp;
}

// 测量稳态输出 RMS（后半段，跳过 attack 收敛期）
static float SteadyRMS(Compressor &c, const std::vector<float> &in)
{
    c.Reset();
    double sum = 0.0;
    int n = 0;
    const int N = (int)in.size();

    for(int i = 0; i < N; i++)
    {
        const float y = c.Process(in[i]);
        if(i >= N / 2) { sum += (double)y * y; n++; }
    }

    return std::sqrt(sum / (double)n);
}

int main()
{
    std::cout << "Compressor Test" << std::endl;
    std::cout << "===============" << std::endl;

    const float SR = 48000.0f;
    const int N = 48000;    // 1 秒

    // 1. 默认构造参数
    {
        Compressor c;
        CheckNear("默认 threshold == -20", c.GetSettings().threshold_db, -20.0f, 0.01f);
        CheckNear("默认 ratio == 4", c.GetSettings().ratio, 4.0f, 0.01f);
    }

    // 2. 低于阈值直通（threshold=-20dB 即幅度 0.1，输入 0.05 不压缩）
    {
        Compressor c(SR, Compressor::Settings{});
        std::vector<float> sq;
        GenSquare(sq, 0.05f, N);
        const float rms = SteadyRMS(c, sq);
        CheckNear("低于阈值直通 RMS ≈ 0.05", rms, 0.05f, 0.002f);
        CheckNear("低于阈值 gain_reduction ≈ 0", c.GetGainReductionDB(), 0.0f, 0.5f);
    }

    // 3. ratio=4:1 压缩（输入 -10dB 峰值 0.316，超阈值 10dB → 只多 2.5dB → 输出 -17.5dB）
    {
        Compressor c(SR, Compressor::Settings{.threshold_db=-20.0f, .ratio=4.0f});
        std::vector<float> sq;
        GenSquare(sq, 0.3162f, N);   // 10^(-10/20)
        const float rms = SteadyRMS(c, sq);
        // 输出幅度 = 0.3162 * 10^(-7.5/20) = 0.1333，方波 RMS = 幅度
        CheckNear("ratio=4:1 输出 RMS ≈ 0.1333", rms, 0.1333f, 0.005f);
        CheckNear("gain_reduction ≈ -7.5dB", c.GetGainReductionDB(), -7.5f, 0.3f);
    }

    // 4. ratio=1 不压缩
    {
        Compressor c(SR, Compressor::Settings{.threshold_db=-20.0f, .ratio=1.0f});
        std::vector<float> sq;
        GenSquare(sq, 0.5f, N);
        const float rms = SteadyRMS(c, sq);
        CheckNear("ratio=1 不压缩 RMS ≈ 0.5", rms, 0.5f, 0.005f);
    }

    // 5. makeup gain +6dB
    {
        Compressor c(SR, Compressor::Settings{.threshold_db=-20.0f, .ratio=4.0f, .makeup_gain_db=6.0f});
        std::vector<float> sq;
        GenSquare(sq, 0.3162f, N);
        const float rms = SteadyRMS(c, sq);
        // 0.1333 * 10^(6/20) = 0.1333 * 1.995 = 0.266
        CheckNear("makeup +6dB 后 RMS ≈ 0.266", rms, 0.266f, 0.01f);
    }

    // 6. limiter 模式（ratio=20，threshold=0dB，输入 1.5 过载 → 输出峰值 ≈ 1.02）
    {
        Compressor c(SR, Compressor::Settings{.threshold_db=0.0f, .ratio=20.0f, .attack_sec=0.001f, .release_sec=0.05f});
        std::vector<float> sq;
        GenSquare(sq, 1.5f, N);
        const float rms = SteadyRMS(c, sq);
        // 输出幅度 ≈ 1.02（被限制在接近 1.0）
        CheckNear("limiter 输出 RMS ≈ 1.02（限制在 ~0dB）", rms, 1.02f, 0.03f);
        Check("limiter 输出峰值未超 1.2", rms < 1.2f);
    }

    // 7. attack/release 平滑（瞬态后渐进，非瞬时跳变）
    {
        Compressor c(SR, Compressor::Settings{.threshold_db=-20.0f, .ratio=4.0f, .attack_sec=0.05f, .release_sec=0.2f});

        // 先稳定在低电平
        for(int i = 0; i < 10000; i++)
            c.Process(0.05f);

        const float gr_before = c.GetGainReductionDB();
        CheckNear("瞬态前 gain_reduction ≈ 0", gr_before, 0.0f, 0.5f);

        // 单一样本跳到高电平，gain_reduction 应只下降一点点（attack 平滑，非瞬时）
        c.Process(1.0f);
        const float gr_after_1 = c.GetGainReductionDB();
        Check("瞬态第 1 样本 gain_reduction 渐进（未到 target）", gr_after_1 > -15.0f);

        // 持续高电平，attack 逐渐逼近 target（-20dB 超 20dB → ratio4 → -15dB）
        for(int i = 0; i < 48000; i++)
            c.Process(1.0f);
        CheckNear("attack 后 gain_reduction ≈ -15dB", c.GetGainReductionDB(), -15.0f, 1.0f);

        // 回到低电平，release 渐进恢复
        c.Process(0.05f);
        const float gr_rel = c.GetGainReductionDB();
        Check("release 渐进（未瞬间归 0）", gr_rel < -1.0f);
    }

    // 8. Reset 清零
    {
        Compressor c(SR, Compressor::Settings{.threshold_db=-20.0f, .ratio=4.0f});
        for(int i = 0; i < 10000; i++)
            c.Process(1.0f);
        c.Reset();
        CheckNear("Reset 后 gain_reduction == 0", c.GetGainReductionDB(), 0.0f, 0.01f);
    }

    // 9. 批量 == 逐个
    {
        Compressor c1(SR, Compressor::Settings{.threshold_db=-20.0f, .ratio=4.0f});
        Compressor c2 = c1;

        std::vector<float> buf(1000);
        for(int i = 0; i < 1000; i++)
            buf[i] = 0.8f * std::sin(2.0f * PI * 440.0f * i / SR);

        c1.Process(buf.data(), 1000);   // 批量

        std::vector<float> single(1000);
        for(int i = 0; i < 1000; i++)
            single[i] = c2.Process(0.8f * std::sin(2.0f * PI * 440.0f * i / SR));

        bool identical = true;
        for(int i = 0; i < 1000; i++)
            if(std::fabs(buf[i] - single[i]) > 1e-5f) identical = false;

        Check("批量 Process == 逐个 Process", identical);
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
