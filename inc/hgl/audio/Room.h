#pragma once

#include<hgl/CoreType.h>
#include<hgl/audio/Ambisonics/AmbisonicTypes.h>
#include<hgl/audio/ReverbPreset.h>
#include<cmath>
#include<algorithm>

namespace hgl::audio
{
    /**
    * 房间墙面材质类型（R4 重构，源自 Amplitude Audio SDK Room，Apache 2.0）
    */
    enum class RoomWallMaterialType
    {
        Transparent=0,
        AcousticTile,
        CarpetOnConcrete,
        HeavyDrapes,
        GypsumBoard,
        ConcreteUnpainted,
        Wood,
        BrickPainted,
        FoamPanel,
        Glass,
        PlasterSmooth,
        Metal,
        Marble,
        WaterSurface,
        IceSurface,
        Custom
    };

    /** 房间墙面 */
    enum class RoomWall
    {
        Left=0,
        Right,
        Floor,
        Ceiling,
        Front,
        Back,
        Invalid
    };

    constexpr int kRoomWallCount=6;

    /**
    * 墙面材质（9 个倍频程吸声系数 125Hz-8kHz）
    */
    class RoomWallMaterial
    {
        float absorption[9]={ 0 };

    public:
        RoomWallMaterial()=default;

        explicit RoomWallMaterial(RoomWallMaterialType type)
        {
            type_=type;
            const float *table=GetAbsorptionTable(type);
            for(int i=0;i<9;i++)
                absorption[i]=table[i];
        }

        RoomWallMaterialType GetType()const{return type_;}

        float GetAbsorption(int band)const{return absorption[band];}
        const float *GetAbsorption()const{return absorption;}

        void SetAbsorption(int band,float value)
        {
            if(band>=0&&band<9)
                absorption[band]=value;
        }

        bool operator==(const RoomWallMaterial &other)const
        {
            for(int i=0;i<9;i++)
                if(absorption[i]!=other.absorption[i])
                    return false;
            return true;
        }

        bool operator!=(const RoomWallMaterial &other)const{return !(*this==other);}

        /** 常用材质吸声系数表（9 频段 125/250/500/1k/2k/4k/8k/16k/32k） */
        static const float *GetAbsorptionTable(RoomWallMaterialType type);

    private:
        RoomWallMaterialType type_=RoomWallMaterialType::Custom;
    };

    /**
    * 房间（R4 重构：精简版，无引擎耦合）
    *
    * 轴对齐盒形声学空间：
    * - 位置 + 尺寸（半宽/深/高）
    * - 6 面墙材质 → 反射系数（sqrt(1-平均吸声)）与 Sabine RT60
    * - 点包含检测（听者/音源室内判定）
    * - 映射到 AudioReverbParams（EFX 混响参数）
    *
    * 概念对应 Amplitude Room + Environment（区域即房间，房间内音源应用房间混响）。
    */
    class Room
    {
    public:
        Room()=default;

        Room(const Vec3 &_location,const Vec3 &_half_dimensions)
            : location(_location),half_dimensions(_half_dimensions)
        {
            std::fill(wall_materials,wall_materials+kRoomWallCount,RoomWallMaterial(RoomWallMaterialType::ConcreteUnpainted));
        }

        void SetLocation(const Vec3 &v){location=v;}
        const Vec3 &GetLocation()const{return location;}

        void SetDimensions(const Vec3 &v){half_dimensions=Vec3{v.x*0.5f,v.y*0.5f,v.z*0.5f};}
        const Vec3 &GetDimensions()const{return half_dimensions;}

        void SetHalfDimensions(const Vec3 &v){half_dimensions=v;}

        void SetWallMaterial(RoomWall wall,const RoomWallMaterial &material)
        {
            const int idx=(int)wall;
            if(idx>=0&&idx<kRoomWallCount)
                wall_materials[idx]=material;
        }

        void SetAllWallMaterials(const RoomWallMaterial &material)
        {
            for(int i=0;i<kRoomWallCount;i++)
                wall_materials[i]=material;
        }

        const RoomWallMaterial &GetWallMaterial(RoomWall wall)const
        {
            return wall_materials[(int)wall];
        }

        /** 点是否在房间内（轴对齐盒） */
        bool Contains(const Vec3 &point)const
        {
            return std::fabs(point.x-location.x)<=half_dimensions.x
                &&std::fabs(point.y-location.y)<=half_dimensions.y
                &&std::fabs(point.z-location.z)<=half_dimensions.z;
        }

        float GetVolume()const
        {
            return 8.0f*half_dimensions.x*half_dimensions.y*half_dimensions.z;
        }

        /** 墙面面积（左/右 = 深×高；上/下 = 宽×深；前/后 = 宽×高） */
        float GetSurfaceArea(RoomWall wall)const
        {
            const float w=2.0f*half_dimensions.x;
            const float d=2.0f*half_dimensions.y;
            const float h=2.0f*half_dimensions.z;

            switch(wall)
            {
            case RoomWall::Left:
            case RoomWall::Right:
                return d*h;
            case RoomWall::Floor:
            case RoomWall::Ceiling:
                return w*d;
            case RoomWall::Front:
            case RoomWall::Back:
                return w*h;
            default:
                return 0.0f;
            }
        }

        /**
        * 每面墙的反射系数 = min(1, sqrt(1-平均吸声))
        * 平均吸声取 500Hz-2kHz 频段（index 4-6，Amplitude 算法）
        */
        float GetReflectionCoefficient(RoomWall wall)const
        {
            const RoomWallMaterial &m=wall_materials[(int)wall];
            const float *abs_coeffs=m.GetAbsorption();

            const float avg=(abs_coeffs[4]+abs_coeffs[5]+abs_coeffs[6])/3.0f;
            const float sqrt_coeff=std::sqrt(1.0f-avg);

            return std::min(1.0f,sqrt_coeff);
        }

        /** 平均反射系数（6 面墙平均） */
        float GetAverageReflectionCoefficient()const
        {
            float sum=0.0f;
            for(int i=0;i<kRoomWallCount;i++)
                sum+=GetReflectionCoefficient((RoomWall)i);
            return sum/(float)kRoomWallCount;
        }

        /**
        * 混响时间 RT60（Sabine 公式：RT60 = 0.161·V / Σ(α_i·S_i)）
        * @return 秒
        */
        float GetRT60()const
        {
            const float volume=GetVolume();
            if(volume<=0.0f)
                return 0.0f;

            float total_absorption=0.0f;
            for(int i=0;i<kRoomWallCount;i++)
            {
                const RoomWall wall=(RoomWall)i;
                const RoomWallMaterial &m=wall_materials[i];
                const float *abs_coeffs=m.GetAbsorption();

                // 500Hz-2kHz 平均吸声（RT60 标准频段）
                const float avg_abs=(abs_coeffs[4]+abs_coeffs[5]+abs_coeffs[6])/3.0f;

                total_absorption+=avg_abs*GetSurfaceArea(wall);
            }

            if(total_absorption<=1e-6f)
                return 0.0f;

            return 0.161f*volume/total_absorption;
        }

        /** 平均吸声系数（500Hz-2kHz，6 面墙按面积加权） */
        float GetAverageAbsorption()const
        {
            float total_area=0.0f;
            float weighted=0.0f;

            for(int i=0;i<kRoomWallCount;i++)
            {
                const float area=GetSurfaceArea((RoomWall)i);
                const RoomWallMaterial &m=wall_materials[i];
                const float *abs_coeffs=m.GetAbsorption();
                const float avg_abs=(abs_coeffs[4]+abs_coeffs[5]+abs_coeffs[6])/3.0f;

                total_area+=area;
                weighted+=avg_abs*area;
            }

            if(total_area<=1e-6f)
                return 0.0f;

            return weighted/total_area;
        }

        /**
        * 房间 → EFX 混响参数（AudioReverbParams）
        *
        * 映射（工程近似）：
        * - DecayTime    = RT60（Sabine）
        * - Density      = 体积归一化（小房间更密）
        * - ReflectionsGain = 平均反射系数（硬表面早期反射强）
        * - LateReverbGain  = 吸声低 → 后期混响强
        */
        AudioReverbParams ToReverbParams()const
        {
            AudioReverbParams p{};

            const float rt60=GetRT60();
            const float volume=GetVolume();
            const float avg_reflection=GetAverageReflectionCoefficient();
            const float avg_absorption=GetAverageAbsorption();

            p.Density=std::clamp(12.0f/volume,0.0f,1.0f);                    // 小房间密度高
            p.Diffusion=0.6f+0.3f*avg_reflection;                            // 硬表面扩散高
            p.Gain=0.5f;
            p.GainHF=0.8f+0.2f*avg_reflection;                               // 硬表面高频保留
            p.GainLF=1.0f;
            p.DecayTime=std::clamp(rt60,0.1f,8.0f);                          // RT60 → 衰减时间
            p.DecayHFRatio=1.0f-0.3f*avg_absorption;                         // 吸声强高频衰减快
            p.DecayLFRatio=1.0f;
            p.ReflectionsGain=0.2f+0.6f*avg_reflection;                      // 早期反射强度
            p.ReflectionsDelay=std::clamp(volume/343.0f*0.25f,0.01f,0.1f);   // 房间尺寸 → 反射延迟
            p.LateReverbGain=0.5f+0.4f*(1.0f-avg_absorption);                // 吸声低 → 混响强
            p.LateReverbDelay=p.ReflectionsDelay*1.5f;
            p.EchoTime=0.05f;
            p.EchoDepth=0.1f;
            p.ModulationTime=0.25f;
            p.ModulationDepth=0.0f;
            p.AirAbsorptionGainHF=0.994f;
            p.HFReference=5000.0f;
            p.LFReference=250.0f;
            p.RoomRolloffFactor=1.0f;
            p.DecayHFLimit=1;

            return p;
        }

    private:
        Vec3 location{ 0.0f,0.0f,0.0f };
        Vec3 half_dimensions{ 5.0f,3.0f,2.5f };     ///< 半宽/深/高
        RoomWallMaterial wall_materials[kRoomWallCount];
    };
}//namespace hgl::audio
