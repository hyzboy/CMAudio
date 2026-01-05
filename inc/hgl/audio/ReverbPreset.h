#ifndef HGL_REVERB_PRESET_INCLUDE
#define HGL_REVERB_PRESET_INCLUDE

#include<hgl/type/BaseString.h>

namespace hgl
{
    /**
     * 混响预设枚举
     */
    enum class ReverbPreset
    {
        Generic = 0,            ///<通用
        PaddedCell,             ///<带衬垫的单元
        Room,                   ///<房间
        Bathroom,               ///<浴室
        LivingRoom,             ///<客厅
        StoneRoom,              ///<石质房间
        Auditorium,             ///<礼堂
        ConcertHall,            ///<音乐厅
        Cave,                   ///<洞穴
        Arena,                  ///<竞技场
        Hangar,                 ///<机库
        CarpetedHallway,        ///<地毯走廊
        Hallway,                ///<走廊
        StoneCorridor,          ///<石质走廊
        Alley,                  ///<小巷
        Forest,                 ///<森林
        City,                   ///<城市
        Mountains,              ///<山脉
        Quarry,                 ///<采石场
        Plain,                  ///<平原
        ParkingLot,             ///<停车场
        SewerPipe,              ///<下水道
        Underwater,             ///<水下
        Drugged,                ///<迷幻
        Dizzy,                  ///<眩晕
        Psychotic,              ///<精神错乱
        
        ENUM_CLASS_RANGE(Generic, Psychotic)
    };

    /**
     * 混响预设属性
     */
    struct ReverbPresetProperties
    {
        UTF8String name_cn;         ///<中文名称
        UTF8String name_en;         ///<英文名称
        
        float flDensity;
        float flDiffusion;
        float flGain;
        float flGainHF;
        float flGainLF;
        float flDecayTime;
        float flDecayHFRatio;
        float flDecayLFRatio;
        float flReflectionsGain;
        float flReflectionsDelay;
        float flReflectionsPan[3];
        float flLateReverbGain;
        float flLateReverbDelay;
        float flLateReverbPan[3];
        float flEchoTime;
        float flEchoDepth;
        float flModulationTime;
        float flModulationDepth;
        float flAirAbsorptionGainHF;
        float flHFReference;
        float flLFReference;
        float flRoomRolloffFactor;
        int   iDecayHFLimit;
    };

    /**
     * 获取混响预设属性
     * @param preset 预设枚举值
     * @return 预设属性指针，如果预设无效则返回nullptr
     */
    const ReverbPresetProperties *GetReverbPresetProperties(ReverbPreset preset);
    
    /**
     * 获取混响预设数量
     */
    int GetReverbPresetCount();
}//namespace hgl
#endif//HGL_REVERB_PRESET_INCLUDE
