// Room Test (R4: 回响环境系统重构验证)
// 1) 吸声系数表 / 反射系数（硬表面 vs 软表面）
// 2) 盒检测（内/外/边界）
// 3) Sabine RT60（尺寸/材质对混响时间的影响）
// 4) 混响参数映射（AudioReverbParams）
#include <iostream>
#include <cmath>
#include <hgl/audio/Room.h>

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
    std::cout << "== Room Test (R4: 回响环境系统) ==" << std::endl;

    // ---- 1. 材质与反射系数 ----
    std::cout << "[1] 材质吸声/反射系数" << std::endl;
    {
        RoomWallMaterial concrete(RoomWallMaterialType::ConcreteUnpainted);
        RoomWallMaterial foam(RoomWallMaterialType::FoamPanel);

        // 混凝土 500Hz-2kHz 平均吸声 (0.03+0.04+0.05)/3=0.04 → 反射 sqrt(0.96)=0.98
        // 泡沫板 500Hz-2kHz 平均 (0.60+0.85+0.90)/3=0.783 → 反射 sqrt(0.217)=0.466
        Check("混凝土高反射（≈0.98）", std::fabs(concrete.GetAbsorption(4)-0.03f)<1e-5f);
        Check("泡沫板高吸声（index4≈0.85）", std::fabs(foam.GetAbsorption(4)-0.85f)<1e-5f);

        Room room(Vec3{0,0,0}, Vec3{5,3,2.5f});
        room.SetAllWallMaterials(concrete);
        const float ref_concrete = room.GetReflectionCoefficient(RoomWall::Front);
        Check("混凝土反射系数高（>0.9）", ref_concrete > 0.9f);

        room.SetAllWallMaterials(foam);
        const float ref_foam = room.GetReflectionCoefficient(RoomWall::Front);
        Check("泡沫板反射系数低（<0.6）", ref_foam < 0.6f);
        Check("混凝土 > 泡沫板反射", ref_concrete > ref_foam);
    }

    // ---- 2. 盒检测 ----
    std::cout << "[2] 室内检测" << std::endl;
    {
        Room room(Vec3{0,0,0}, Vec3{5,3,2.5f});

        Check("室内点", room.Contains(Vec3{1,1,1}));
        Check("边界点", room.Contains(Vec3{5,3,2.5f}));
        Check("室外点", !room.Contains(Vec3{6,0,0}));
        Check("上方点", !room.Contains(Vec3{0,0,3}));

        room.SetLocation(Vec3{100,0,0});
        Check("平移后室内点", room.Contains(Vec3{101,0,0}));
        Check("平移后室外点", !room.Contains(Vec3{0,0,0}));
    }

    // ---- 3. Sabine RT60 ----
    std::cout << "[3] RT60（Sabine）" << std::endl;
    {
        RoomWallMaterial concrete(RoomWallMaterialType::ConcreteUnpainted);
        RoomWallMaterial foam(RoomWallMaterialType::FoamPanel);

        // 同一房间：混凝土（反射强）→ RT60 长；泡沫板（吸声强）→ RT60 短
        Room room(Vec3{0,0,0}, Vec3{5,3,2.5f});   // 10×6×5 m

        room.SetAllWallMaterials(concrete);
        const float rt60_concrete = room.GetRT60();

        room.SetAllWallMaterials(foam);
        const float rt60_foam = room.GetRT60();

        std::cout << "    RT60(混凝土)=" << rt60_concrete << "s  RT60(泡沫板)=" << rt60_foam << "s" << std::endl;
        Check("混凝土 RT60 合理（1-10s）", rt60_concrete > 1.0f && rt60_concrete < 10.0f);
        Check("泡沫板 RT60 短（<1s）", rt60_foam < 1.0f);
        Check("混凝土 RT60 > 泡沫板 RT60", rt60_concrete > rt60_foam*3.0f);

        // 同材质（混凝土）：小房间 RT60 短于大房间
        Room big(Vec3{0,0,0}, Vec3{5,3,2.5f});    // 10×6×5 m
        big.SetAllWallMaterials(concrete);

        Room small(Vec3{0,0,0}, Vec3{2,2,1.5f});   // 4×4×3 m
        small.SetAllWallMaterials(concrete);
        Check("小房间 RT60 < 大房间", small.GetRT60() < big.GetRT60());
    }

    // ---- 4. 混响参数映射 ----
    std::cout << "[4] 房间 → 混响参数" << std::endl;
    {
        RoomWallMaterial concrete(RoomWallMaterialType::ConcreteUnpainted);
        Room room(Vec3{0,0,0}, Vec3{5,3,2.5f});
        room.SetAllWallMaterials(concrete);

        AudioReverbParams p = room.ToReverbParams();

        Check("DecayTime = RT60", std::fabs(p.DecayTime - room.GetRT60()) < 0.5f);
        Check("Density 合理（0-1）", p.Density >= 0.0f && p.Density <= 1.0f);
        Check("ReflectionsGain 合理（0-1）", p.ReflectionsGain >= 0.0f && p.ReflectionsGain <= 1.0f);
        Check("LateReverbGain 合理（0-1）", p.LateReverbGain >= 0.0f && p.LateReverbGain <= 1.0f);
        Check("ReflectionsDelay > 0", p.ReflectionsDelay > 0.0f);

        // 软表面房间：混响弱（LateReverbGain 低）
        Room soft_room(Vec3{0,0,0}, Vec3{5,3,2.5f});
        soft_room.SetAllWallMaterials(RoomWallMaterial(RoomWallMaterialType::FoamPanel));

        AudioReverbParams p_soft = soft_room.ToReverbParams();
        Check("硬表面后期混响 > 软表面", p.LateReverbGain > p_soft.LateReverbGain);
    }

    std::cout << "== 结果: " << (failed ? "FAILED" : "ALL PASSED") << " (" << failed << " failures) ==" << std::endl;
    return failed ? 1 : 0;
}
