#include<hgl/audio/Room.h>

namespace hgl::audio
{
    // 常用材质吸声系数表（9 频段：125/250/500/1k/2k/4k/8k/16k/32k Hz）
    // 源自 Amplitude Audio SDK RoomInternalState（Apache 2.0）
    const float *RoomWallMaterial::GetAbsorptionTable(RoomWallMaterialType type)
    {
        static const float kTables[(int)RoomWallMaterialType::Custom][9]={
            /* Transparent       */ { 1.00f,1.00f,1.00f,1.00f,1.00f,1.00f,1.00f,1.00f,1.00f },
            /* AcousticTile      */ { 0.11f,0.21f,0.41f,0.71f,0.76f,0.86f,0.86f,0.91f,0.91f },
            /* CarpetOnConcrete  */ { 0.03f,0.06f,0.11f,0.16f,0.21f,0.26f,0.31f,0.41f,0.61f },
            /* HeavyDrapes       */ { 0.15f,0.36f,0.56f,0.71f,0.71f,0.66f,0.61f,0.51f,0.36f },
            /* GypsumBoard       */ { 0.21f,0.11f,0.06f,0.11f,0.06f,0.05f,0.08f,0.10f,0.11f },
            /* ConcreteUnpainted */ { 0.01f,0.01f,0.02f,0.02f,0.03f,0.04f,0.05f,0.07f,0.09f },
            /* Wood              */ { 0.29f,0.23f,0.18f,0.10f,0.11f,0.08f,0.10f,0.09f,0.11f },
            /* BrickPainted      */ { 0.03f,0.03f,0.03f,0.04f,0.05f,0.04f,0.05f,0.07f,0.09f },
            /* FoamPanel         */ { 0.15f,0.30f,0.45f,0.60f,0.85f,0.90f,0.95f,0.95f,0.90f },
            /* Glass             */ { 0.07f,0.06f,0.05f,0.04f,0.03f,0.02f,0.02f,0.02f,0.02f },
            /* PlasterSmooth     */ { 0.03f,0.03f,0.04f,0.04f,0.05f,0.05f,0.04f,0.05f,0.06f },
            /* Metal             */ { 0.01f,0.01f,0.01f,0.01f,0.02f,0.02f,0.03f,0.03f,0.03f },
            /* Marble            */ { 0.01f,0.01f,0.01f,0.02f,0.02f,0.02f,0.03f,0.04f,0.05f },
            /* WaterSurface      */ { 0.01f,0.01f,0.01f,0.02f,0.02f,0.03f,0.04f,0.05f,0.06f },
            /* IceSurface        */ { 0.01f,0.01f,0.02f,0.02f,0.03f,0.03f,0.04f,0.05f,0.06f },
        };

        const int idx=(int)type;
        if(idx<0||idx>=(int)RoomWallMaterialType::Custom)
            return kTables[0];

        return kTables[idx];
    }
}//namespace hgl::audio
