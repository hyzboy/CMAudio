// HRIRSphere Test (R3: .amir 数据加载 + 方向采样)
// 1) 加载 sadie_h12.amir（真实 SADIE II HRTF 数据）
// 2) 验证头部（采样率/IR 长度/顶点/面数）
// 3) 方向采样：正前方/左侧 → 左右耳 HRIR 能量（ITD 效应）
// 4) 与 AmbisonicBinauralizer 集成（真实 HRTF 双耳化）
#include <iostream>
#include <cmath>
#include <vector>
#include <hgl/audio/HRTF/HRIRSphere.h>
#include <hgl/audio/Ambisonics/AmbisonicBinauralizer.h>
#include <hgl/audio/Ambisonics/AmbisonicSource.h>

using namespace hgl;
using namespace hgl::audio;

static int failed = 0;

static void Check(const char *name, bool cond)
{
    std::cout << (cond ? "  [PASS] " : "  [FAIL] ") << name << std::endl;
    if(!cond) ++failed;
}

static float Energy(const float *buf, uint32 n)
{
    float e = 0;
    for(uint32 i=0;i<n;i++) e += buf[i]*buf[i];
    return e;
}

int main()
{
    std::cout << "== HRIRSphere Test (R3: .amir 加载与采样) ==" << std::endl;

    HRIRSphereImpl hrir;

    // ---- 1. 加载 ----
    std::cout << "[1] 加载 sadie_h12.amir" << std::endl;
    Check("LoadFromFile", hrir.LoadFromFile("sadie_h12.amir"));
    Check("IsLoaded", hrir.IsLoaded());

    // ---- 2. 头部 ----
    std::cout << "[2] 数据规格" << std::endl;
    std::cout << "    采样率=" << hrir.GetSampleRate()
              << " IR长度=" << hrir.GetIRLength()
              << " 顶点=" << hrir.GetVertexCount()
              << " 面=" << hrir.GetFaceCount() << std::endl;
    Check("采样率合理（44.1k/48k）", hrir.GetSampleRate()==44100||hrir.GetSampleRate()==48000);
    Check("IR 长度合理（64-1024）", hrir.GetIRLength()>=64&&hrir.GetIRLength()<=1024);
    Check("顶点数 > 0", hrir.GetVertexCount()>0);
    Check("面数 > 0", hrir.GetFaceCount()>0);

    // ---- 3. 方向采样 ----
    std::cout << "[3] 方向采样（最近邻）" << std::endl;
    {
        const uint32 len = hrir.GetIRLength();
        std::vector<float> l(len), r(len);

        // 正前方：左右耳能量同量级（真人 HRTF 头不对称 + 网格最近邻误差，允许 25% 偏差）
        hrir.Sample(Vec3{1,0,0}, l.data(), r.data());
        float el = Energy(l.data(), len);
        float er = Energy(r.data(), len);
        Check("正前方左右耳能量相近（<25%）", std::fabs(el-er) < 0.25f*(el+er+1e-12f));
        Check("正前方有能量", el > 1e-6f);

        // 正左侧：左耳能量 > 右耳（头影效应）
        hrir.Sample(Vec3{0,1,0}, l.data(), r.data());
        el = Energy(l.data(), len);
        er = Energy(r.data(), len);
        Check("左侧左耳能量 > 右耳（头影）", el > er*1.1f);
    }

    // ---- 4. 双线性插值模式 ----
    std::cout << "[4] 双线性插值" << std::endl;
    {
        hrir.SetSamplingMode(HRIRSphere::SamplingMode::Bilinear);
        Check("采样模式设置", hrir.GetSamplingMode()==HRIRSphere::SamplingMode::Bilinear);

        const uint32 len = hrir.GetIRLength();
        std::vector<float> l(len), r(len);
        hrir.Sample(Vec3{1,0,0}, l.data(), r.data());
        Check("双线性正前方有能量", Energy(l.data(), len) > 1e-6f);

        hrir.SetSamplingMode(HRIRSphere::SamplingMode::NearestNeighbor);
    }

    // ---- 5. 与 Binauralizer 集成（真实 HRTF）----
    std::cout << "[5] 真实 HRTF 双耳化" << std::endl;
    {
        AmbisonicBinauralizer binaural;
        Check("Configure（真实 HRIR）", binaural.Configure(1, true, 128, hrir.GetSampleRate(), &hrir));

        // 编码左侧声源（90°）
        AmbisonicSource source;
        source.Configure(1, true);
        source.SetPosition(SphericalPosition(90.0f*Amb_DegToRad, 0.0f, 1.0f));

        BFormat bf;
        bf.Configure(1, true, 128);

        AudioChannel mono;
        mono.resize(128);
        for(int i=0;i<128;i++) mono[i] = std::sin(2.0f*3.14159265358979f*440.0f*i/hrir.GetSampleRate());

        source.Process(mono, 128, &bf);

        AmbisonicBuffer out(128, 2);
        binaural.Process(&bf, 128, out);

        float el = 0, er = 0;
        for(int i=0;i<128;i++) { el += out[0][i]*out[0][i]; er += out[1][i]*out[1][i]; }

        Check("双耳输出有能量", el+er > 1e-6f);
        Check("左侧声源 → 左耳能量 > 右耳", el > er*1.05f);
        std::cout << "    左耳能量=" << el << " 右耳能量=" << er << std::endl;
    }

    std::cout << "== 结果: " << (failed ? "FAILED" : "ALL PASSED") << " (" << failed << " failures) ==" << std::endl;
    return failed ? 1 : 0;
}
