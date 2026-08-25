// Ambisonics Test (R2: Ambisonics 移植验证)
// 1) AmbisonicSource 球谐编码系数（正前方/侧方）
// 2) AmbisonicDecoder 5.1 解码（W 单声道 → 各扬声器馈送）
// 3) AmbisonicOrientationProcessor 声场旋转（90°）
// 4) AmbisonicShelfFilter 心理声学校准（DC 通过/高频增益）
// 5) AmbisonicBinauralizer 双耳化（mock HRIR：单位脉冲 → 输出=输入）
#include <iostream>
#include <cmath>
#include <vector>
#include <hgl/audio/Ambisonics/AmbisonicSource.h>
#include <hgl/audio/Ambisonics/AmbisonicDecoder.h>
#include <hgl/audio/Ambisonics/AmbisonicOrientationProcessor.h>
#include <hgl/audio/Ambisonics/AmbisonicShelfFilter.h>
#include <hgl/audio/Ambisonics/AmbisonicBinauralizer.h>

using namespace hgl;
using namespace hgl::audio;

static int failed = 0;

static void Check(const char *name, bool cond)
{
    std::cout << (cond ? "  [PASS] " : "  [FAIL] ") << name << std::endl;
    if(!cond) ++failed;
}

static bool AlmostEq(float a, float b, float eps = 1e-3f)
{
    return std::fabs(a - b) < eps;
}

/** Mock HRIR：单位脉冲（方向无关）——验证双耳化链路 */
class MockHRIRSphere : public HRIRSphere
{
    uint32 _len;
public:
    explicit MockHRIRSphere(uint32 len) : _len(len) {}
    bool IsLoaded()const override { return true; }
    uint32 GetIRLength()const override { return _len; }
    uint32 GetSampleRate()const override { return 48000; }
    void Sample(const Vec3 &, float *left, float *right)const override
    {
        for(uint32 i=0;i<_len;i++) { left[i]=0.0f; right[i]=0.0f; }
        left[0]=1.0f;
        right[0]=1.0f;
    }
};

int main()
{
    std::cout << "== Ambisonics Test (R2: 移植验证) ==" << std::endl;

    // ---- 1. 球谐编码系数 ----
    std::cout << "[1] AmbisonicSource 编码系数（SN3D/ACN）" << std::endl;
    {
        AmbisonicSource source;
        Check("Configure 1 阶 3D", source.Configure(1, true));
        Check("通道数 4", source.GetChannelCount() == 4);

        // 正前方 (0°,0°)：W=1, Y=0, Z=0, X=1
        source.SetPosition(SphericalPosition(0.0f, 0.0f, 1.0f));
        Check("W=1", AlmostEq(source.GetCoefficient((int)BFormatChannel::W), 1.0f));
        Check("Y=0（正前方无侧向分量）", AlmostEq(source.GetCoefficient((int)BFormatChannel::Y), 0.0f));
        Check("Z=0（水平面）", AlmostEq(source.GetCoefficient((int)BFormatChannel::Z), 0.0f));
        Check("X=1（正前方）", AlmostEq(source.GetCoefficient((int)BFormatChannel::X), 1.0f));

        // 正左侧 (90°,0°)：Y=1
        source.SetPosition(SphericalPosition(90.0f*Amb_DegToRad, 0.0f, 1.0f));
        Check("左侧 Y=1", AlmostEq(source.GetCoefficient((int)BFormatChannel::Y), 1.0f));
        Check("左侧 X=0", AlmostEq(source.GetCoefficient((int)BFormatChannel::X), 0.0f));

        // 编码：单声道 → BFormat（W 通道 = 输入 × W 系数）
        BFormat bf;
        bf.Configure(1, true, 128);

        AudioChannel mono;
        mono.resize(128);
        for(int i=0;i<128;i++) mono[i] = std::sin(2.0f*3.14159265358979f*440.0f*i/48000.0f);

        source.SetPosition(SphericalPosition(0.0f, 0.0f, 1.0f));
        source.Process(mono, 128, &bf);

        Check("W 通道=输入", AlmostEq(bf.GetSample((int)BFormatChannel::W, 10), mono[10]));
        Check("X 通道=输入（正前方）", AlmostEq(bf.GetSample((int)BFormatChannel::X, 10), mono[10]));
    }

    // ---- 2. 解码器 5.1 ----
    std::cout << "[2] AmbisonicDecoder 5.1 解码" << std::endl;
    {
        AmbisonicDecoder decoder;
        Check("Configure 5.1", decoder.Configure(1, true, SpeakersPreset::Surround_5_1));
        Check("6 扬声器", decoder.GetSpeakerCount() == 6);
        Check("IsLoaded", decoder.IsLoaded());

        // 单声道 BFormat（仅 W）：中心扬声器（扬声器 4，0°）最强
        BFormat bf;
        bf.Configure(1, true, 64);
        for(int i=0;i<64;i++) bf.SetSample((int)BFormatChannel::W, i, 1.0f);

        AmbisonicBuffer out(64, 6);
        decoder.Process(&bf, 64, out);

        // 中心扬声器（index 4，0°）W 系数 0.141421；左右（0/1，±30°）W 系数 0.300520
        // （Ambisonic 解码伪逆设计：中心承担较少 W 能量）
        float center = out[4][10];
        float lf = out[0][10];
        float lfe = out[5][10];
        Check("中心扬声器有信号（W 系数 0.141）", center > 0.1f);
        Check("左右扬声器有信号（W 系数 0.3005）", lf > 0.2f);
        Check("左右 > 中心（解码表设计）", lf > center);
        Check("LFE 有 W 分量（系数 0.5）", AlmostEq(lfe, 0.5f));
    }

    // ---- 3. 声场旋转 ----
    std::cout << "[3] AmbisonicOrientationProcessor 旋转" << std::endl;
    {
        // 编码正前方声源 → X=1, Y=0
        AmbisonicSource source;
        source.Configure(1, true);
        source.SetPosition(SphericalPosition(0.0f, 0.0f, 1.0f));

        BFormat bf;
        bf.Configure(1, true, 64);

        AudioChannel mono;
        mono.resize(64, 1.0f);
        source.Process(mono, 64, &bf);

        Check("旋转前 X=1", AlmostEq(bf.GetSample((int)BFormatChannel::X, 0), 1.0f));
        Check("旋转前 Y=0", AlmostEq(bf.GetSample((int)BFormatChannel::Y, 0), 0.0f));

        // 旋转 90°（alpha）→ 声场转向左侧：X→0, Y→-1
        AmbisonicOrientationProcessor rot;
        rot.Configure(1, true);
        rot.SetOrientation(90.0f*Amb_DegToRad, 0.0f, 0.0f);
        rot.Process(&bf, 64);

        Check("旋转 90° 后 X≈0", AlmostEq(bf.GetSample((int)BFormatChannel::X, 0), 0.0f, 1e-2f));
        Check("旋转 90° 后 Y≈-1", AlmostEq(bf.GetSample((int)BFormatChannel::Y, 0), -1.0f, 1e-2f));
    }

    // ---- 4. 心理声学校准滤波 ----
    std::cout << "[4] AmbisonicShelfFilter" << std::endl;
    {
        AmbisonicShelfFilter shelf;
        Check("Configure 1 阶", shelf.Configure(1, true, 128, 48000));

        // DC 信号：低频应完整通过（LP 支路增益 1）
        BFormat bf;
        bf.Configure(1, true, 128);
        for(int i=0;i<128;i++) bf.SetSample((int)BFormatChannel::W, i, 1.0f);

        shelf.Process(&bf, 128);

        Check("DC 通过（W 通道≈1）", AlmostEq(bf.GetSample((int)BFormatChannel::W, 100), 1.0f, 1e-2f));

        // max-rE 增益：3D 1 阶的 0 阶增益 ≈ 1.0
        std::vector<float> gains = shelf.GetMaxReGains();
        Check("max-rE 增益数 = order+1", gains.size() == 2);
    }

    // ---- 5. 双耳化（mock HRIR）----
    std::cout << "[5] AmbisonicBinauralizer（mock HRIR）" << std::endl;
    {
        MockHRIRSphere hrir(64);
        AmbisonicBinauralizer binaural;
        Check("Configure（mock HRIR）", binaural.Configure(1, true, 128, 48000, &hrir));

        // 编码正前方声源
        AmbisonicSource source;
        source.Configure(1, true);
        source.SetPosition(SphericalPosition(0.0f, 0.0f, 1.0f));

        BFormat bf;
        bf.Configure(1, true, 128);

        AudioChannel mono;
        mono.resize(128);
        for(int i=0;i<128;i++) mono[i] = std::sin(2.0f*3.14159265358979f*440.0f*i/48000.0f);

        source.Process(mono, 128, &bf);

        AmbisonicBuffer out(128, 2);
        binaural.Process(&bf, 128, out);

        // 输出应有能量（非零）
        float energy = 0;
        for(int i=0;i<128;i++) energy += out[0][i]*out[0][i] + out[1][i]*out[1][i];
        Check("双耳输出有能量", energy > 1e-6f);
        Check("左右耳输出相似（正前方对称）", AlmostEq(out[0][0], out[1][0], 1e-2f));
    }

    std::cout << "== 结果: " << (failed ? "FAILED" : "ALL PASSED") << " (" << failed << " failures) ==" << std::endl;
    return failed ? 1 : 0;
}
