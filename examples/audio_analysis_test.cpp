// AudioAnalysis Test
// 验证 FFT / RMS / dB / 幅度谱 / 谱通量 / onset 检测（纯数学，无需 OpenAL）
#include <iostream>
#include <cmath>
#include <vector>
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

int main()
{
    std::cout << "AudioAnalysis Test" << std::endl;
    std::cout << "==================" << std::endl;

    // 1. dB 转换
    CheckNear("LinearToDB(1.0) == 0",      LinearToDB(1.0f),   0.0f,     0.0001f);
    CheckNear("LinearToDB(0.5) == -6.02",  LinearToDB(0.5f),   -6.0206f, 0.001f);
    CheckNear("LinearToDB(0) == -120",     LinearToDB(0.0f),   -120.0f,  0.0001f);
    CheckNear("DBToLinear 往返",           DBToLinear(LinearToDB(0.25f)), 0.25f, 0.0001f);

    // 2. RMS
    {
        std::vector<float> s;
        for(int i = 0; i < 48000; i++)
            s.push_back(std::sin(2.0 * PI * i / 48.0));
        CheckNear("满幅正弦 RMS == 0.7071",  ComputeRMS(s.data(), (int)s.size()),  0.7071f, 0.001f);
        CheckNear("满幅正弦 RMSdB == -3.01", ComputeRMSdB(s.data(), (int)s.size()), -3.0103f, 0.01f);
    }
    {
        float s[8] = {1,1,1,1,1,1,1,1};
        CheckNear("直流 RMS == 1.0", ComputeRMS(s, 8), 1.0f, 0.0001f);
    }

    // 3. Peak
    {
        float s[4] = {-0.3f, 0.5f, -0.9f, 0.1f};
        CheckNear("Peak == 0.9", ComputePeak(s, 4), 0.9f, 0.0001f);
        CheckNear("PeakdB == -0.915", ComputePeakdB(s, 4), -0.9151f, 0.01f);
    }

    // 4. FFT：DC 信号
    {
        int n = 8;
        float re[8] = {1,1,1,1,1,1,1,1};
        float im[8] = {0,0,0,0,0,0,0,0};
        Check("FFT(n=8) 成功", FFT(re, im, n));
        CheckNear("DC: re[0] == 8", re[0], 8.0f, 0.001f);
        bool rest_zero = true;
        for(int i = 1; i < n; i++)
            if(std::fabs(re[i]) > 0.001f || std::fabs(im[i]) > 0.001f) rest_zero = false;
        Check("DC: 其余 bin == 0", rest_zero);
    }

    // 5. FFT：单一正弦 bin k=2，n=8，幅度 1
    //    sin(2*pi*2*i/8) -> X[2] = -j*4, X[6] = +j*4
    {
        int n = 8;
        float re[8], im[8];
        for(int i = 0; i < n; i++) { re[i] = std::sin(2.0f * PI * 2.0f * i / n); im[i] = 0.0f; }
        Check("FFT 正弦成功", FFT(re, im, n));
        CheckNear("正弦 bin2 re == 0",  re[2], 0.0f, 0.001f);
        CheckNear("正弦 bin2 im == -4", im[2], -4.0f, 0.001f);
        CheckNear("正弦 bin6 im == +4", im[6], 4.0f, 0.001f);
    }

    // 6. Parseval 定理：sum(|X|^2) == n * sum(|x|^2)
    {
        int n = 16;
        float re[16], im[16];
        double time_energy = 0.0;
        for(int i = 0; i < n; i++)
        {
            re[i] = 0.3f * std::sin(2.0f * PI * 3.0f * i / n) + 0.1f;
            im[i] = 0.0f;
            time_energy += (double)re[i] * re[i];
        }
        FFT(re, im, n);
        double freq_energy = 0.0;
        for(int i = 0; i < n; i++)
            freq_energy += (double)re[i] * re[i] + (double)im[i] * im[i];
        CheckNear("Parseval: sum|X|^2/n == sum|x|^2", (float)(freq_energy / n), (float)time_energy, 0.01f);
    }

    // 7. 幅度谱：正弦 bin2 幅度 1 -> magnitude[2] == 0.5
    {
        int n = 8;
        float s[8];
        for(int i = 0; i < n; i++) s[i] = std::sin(2.0f * PI * 2.0f * i / n);
        float mag[8];
        Check("ComputeMagnitudeSpectrum 成功", ComputeMagnitudeSpectrum(s, n, mag, 8));
        CheckNear("幅度谱 bin2 == 0.5", mag[2], 0.5f, 0.001f);
        CheckNear("幅度谱 DC == 0",    mag[0], 0.0f, 0.001f);
    }

    // 8. 谱通量
    {
        float prev[4] = {0,1,2,3};
        float cur [4] = {0,1,4,3};   // 正差仅 bin2=2
        CheckNear("SpectralFlux == 2", ComputeSpectralFlux(cur, prev, 4), 2.0f, 0.0001f);
        float cur2[4] = {0,0,0,0};   // 全负差 -> 0
        CheckNear("SpectralFlux 负变化 == 0", ComputeSpectralFlux(cur2, prev, 4), 0.0f, 0.0001f);
    }

    // 9. OnsetDetector
    {
        OnsetDetector det(1.5f);
        Check("首帧不是 onset",    !det.Detect(0.10f));
        Check("能量突增是 onset",  det.Detect(0.30f));   // 0.30 > 0.10 * 1.5
        Check("能量平稳不是 onset", !det.Detect(0.25f)); // 0.25 < 0.30 * 1.5
        det.Reset();
        Check("Reset 后首帧不是 onset", !det.Detect(0.90f));
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
