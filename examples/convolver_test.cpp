// Convolver Test (R1: FFT/Convolver 移植验证)
// 1) FFT 往返（Forward→Backward 恢复原信号）
// 2) 单位脉冲 IR → 卷积输出 = 输入
// 3) 随机 IR 与直接时域卷积对比（多 block 分段正确性）
#include <iostream>
#include <cmath>
#include <cstring>
#include <vector>
#include <random>
#include <hgl/audio/AudioFFT.h>
#include <hgl/audio/Convolver.h>

using namespace hgl;
using namespace hgl::audio;

static int failed = 0;

static void Check(const char *name, bool cond)
{
    std::cout << (cond ? "  [PASS] " : "  [FAIL] ") << name << std::endl;
    if(!cond) ++failed;
}

static bool AlmostEq(float a, float b, float eps)
{
    return std::fabs(a - b) < eps;
}

int main()
{
    std::cout << "== Convolver Test (R1: FFT/Convolver 移植) ==" << std::endl;

    // ---- 1. FFT 往返 ----
    std::cout << "[1] FFT 往返" << std::endl;
    {
        const int N = 1024;
        AudioFFT fft;
        fft.Initialize(N);

        std::vector<float> input(N);
        for(int i=0;i<N;i++)
            input[i] = std::sin(2.0f*3.14159265358979f*50.0f*i/N) + 0.3f*std::sin(2.0f*3.14159265358979f*200.0f*i/N);

        SplitComplex spec;
        fft.Forward(input.data(), spec);
        Check("GetOutputSize == N/2+1", spec.GetSize() == N/2+1);

        std::vector<float> output(N);
        fft.Backward(output.data(), spec);

        // 往返误差（PFFFT 逆变换自带 1/N）
        float max_err = 0;
        for(int i=0;i<N;i++)
            max_err = std::max(max_err, std::fabs(output[i]-input[i]));

        Check("FFT 往返误差 < 1e-4", max_err < 1e-4f);
        std::cout << "    max_err=" << max_err << std::endl;
    }

    // ---- 2. 单位脉冲 IR → 输出=输入 ----
    std::cout << "[2] 单位脉冲 IR" << std::endl;
    {
        Convolver conv;
        const float ir[1] = {1.0f};
        Check("Init 成功", conv.Init(512, ir, 1));
        Check("段数 1", conv.GetSegmentCount() == 1);

        const int N = 2048;
        std::vector<float> in(N), out(N);
        for(int i=0;i<N;i++)
            in[i] = std::sin(2.0f*3.14159265358979f*100.0f*i/16000.0f);

        conv.Process(in.data(), out.data(), N);

        float max_err = 0;
        for(int i=0;i<N;i++)
            max_err = std::max(max_err, std::fabs(out[i]-in[i]));

        Check("单位脉冲输出=输入（<1e-3）", max_err < 1e-3f);
    }

    // ---- 3. 随机 IR vs 直接时域卷积 ----
    std::cout << "[3] 随机 IR 与直接卷积对比（分段正确性）" << std::endl;
    {
        const int IR_LEN = 2000;        // 跨多段（block=512 → 4 段）
        const int N = 4096;

        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(-0.5f, 0.5f);

        std::vector<float> ir(IR_LEN), in(N), out(N), expect(N, 0.0f);
        for(int i=0;i<IR_LEN;i++) ir[i] = dist(rng);
        for(int i=0;i<N;i++) in[i] = dist(rng);

        Convolver conv;
        Check("Init 成功", conv.Init(512, ir.data(), IR_LEN));
        Check("段数 ceil(2000/512)=4", conv.GetSegmentCount() == 4);

        conv.Process(in.data(), out.data(), N);

        // 直接时域卷积（线性卷积，输出前 N 点）
        for(int i=0;i<N;i++)
        {
            float sum = 0;
            const int k_max = std::min(i+1, IR_LEN);
            for(int k=0;k<k_max;k++)
                sum += in[i-k] * ir[k];
            expect[i] = sum;
        }

        float max_err = 0;
        for(int i=0;i<N;i++)
        {
            const float e = std::fabs(out[i]-expect[i]);
            if(e > max_err) { max_err = e; }
        }

        Check("分段卷积 ≈ 直接卷积（<1e-3）", max_err < 1e-3f);
        std::cout << "    max_err=" << max_err << std::endl;
    }

    // ---- 4. 流式输入（多次小块 Process）----
    std::cout << "[4] 流式输入（分块处理等价于一次性）" << std::endl;
    {
        const int IR_LEN = 100;
        const int N = 2048;

        std::mt19937 rng(7);
        std::uniform_real_distribution<float> dist(-0.5f, 0.5f);

        std::vector<float> ir(IR_LEN), in(N), out1(N), out2(N);
        for(int i=0;i<IR_LEN;i++) ir[i] = dist(rng);
        for(int i=0;i<N;i++) in[i] = dist(rng);

        // 一次性
        Convolver c1;
        c1.Init(256, ir.data(), IR_LEN);
        c1.Process(in.data(), out1.data(), N);

        // 分块（每块 123 个——非对齐块大小）
        Convolver c2;
        c2.Init(256, ir.data(), IR_LEN);
        int pos = 0;
        while(pos < N)
        {
            const int chunk = std::min(123, N-pos);
            c2.Process(in.data()+pos, out2.data()+pos, chunk);
            pos += chunk;
        }

        float max_err = 0;
        for(int i=0;i<N;i++)
            max_err = std::max(max_err, std::fabs(out1[i]-out2[i]));

        Check("分块处理等价于一次性（<1e-4）", max_err < 1e-4f);
        std::cout << "    max_err=" << max_err << std::endl;
    }

    std::cout << "== 结果: " << (failed ? "FAILED" : "ALL PASSED") << " (" << failed << " failures) ==" << std::endl;
    return failed ? 1 : 0;
}
