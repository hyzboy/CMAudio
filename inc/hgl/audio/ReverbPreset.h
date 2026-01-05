#pragma once

#include<hgl/type/String.h>

namespace hgl
{
    /**
     * 混响预设枚举 - 包含所有 113 个 OpenAL Soft 官方预设
     */
    enum class ReverbPreset
    {
        Generic,                    ///< 通用
        Paddedcell,                 ///< 带衬垫的单元
        Room,                       ///< 房间
        Bathroom,                   ///< 浴室
        Livingroom,                 ///< 客厅
        Stoneroom,                  ///< 石质房间
        Auditorium,                 ///< 礼堂
        Concerthall,                ///< 音乐厅
        Cave,                       ///< 洞穴
        Arena,                      ///< 竞技场
        Hangar,                     ///< 机库
        Carpetedhallway,            ///< 地毯走廊
        Hallway,                    ///< 走廊
        Stonecorridor,              ///< 石质走廊
        Alley,                      ///< 小巷
        Forest,                     ///< 森林
        City,                       ///< 城市
        Mountains,                  ///< 山脉
        Quarry,                     ///< 采石场
        Plain,                      ///< 平原
        Parkinglot,                 ///< 停车场
        Sewerpipe,                  ///< 下水道
        Underwater,                 ///< 水下
        Drugged,                    ///< 迷幻
        Dizzy,                      ///< 眩晕
        Psychotic,                  ///< 精神错乱
        CastleSmallroom,            ///< 城堡小房间
        CastleShortpassage,         ///< 城堡短通道
        CastleMediumroom,           ///< 城堡中房间
        CastleLargeroom,            ///< 城堡大房间
        CastleLongpassage,          ///< 城堡长通道
        CastleHall,                 ///< 城堡大厅
        CastleCupboard,             ///< 城堡橱柜
        CastleCourtyard,            ///< 城堡庭院
        CastleAlcove,               ///< 城堡壁龛
        FactorySmallroom,           ///< 工厂小房间
        FactoryShortpassage,        ///< 工厂短通道
        FactoryMediumroom,          ///< 工厂中房间
        FactoryLargeroom,           ///< 工厂大房间
        FactoryLongpassage,         ///< 工厂长通道
        FactoryHall,                ///< 工厂大厅
        FactoryCupboard,            ///< 工厂橱柜
        FactoryCourtyard,           ///< 工厂庭院
        FactoryAlcove,              ///< 工厂壁龛
        IcepalaceSmallroom,         ///< 冰宫小房间
        IcepalaceShortpassage,      ///< 冰宫短通道
        IcepalaceMediumroom,        ///< 冰宫中房间
        IcepalaceLargeroom,         ///< 冰宫大房间
        IcepalaceLongpassage,       ///< 冰宫长通道
        IcepalaceHall,              ///< 冰宫大厅
        IcepalaceCupboard,          ///< 冰宫橱柜
        IcepalaceCourtyard,         ///< 冰宫庭院
        IcepalaceAlcove,            ///< 冰宫壁龛
        SpacestationSmallroom,      ///< 空间站小房间
        SpacestationShortpassage,   ///< 空间站短通道
        SpacestationMediumroom,     ///< 空间站中房间
        SpacestationLargeroom,      ///< 空间站大房间
        SpacestationLongpassage,    ///< 空间站长通道
        SpacestationHall,           ///< 空间站大厅
        SpacestationCupboard,       ///< 空间站橱柜
        SpacestationAlcove,         ///< 空间站壁龛
        WoodenSmallroom,            ///< 木质小房间
        WoodenShortpassage,         ///< 木质短通道
        WoodenMediumroom,           ///< 木质中房间
        WoodenLargeroom,            ///< 木质大房间
        WoodenLongpassage,          ///< 木质长通道
        WoodenHall,                 ///< 木质大厅
        WoodenCupboard,             ///< 木质橱柜
        WoodenCourtyard,            ///< 木质庭院
        WoodenAlcove,               ///< 木质壁龛
        SportEmptystadium,          ///< 运动场空体育场
        SportSquashcourt,           ///< 运动场壁球场
        SportSmallswimmingpool,     ///< 运动场小泳池
        SportLargeswimmingpool,     ///< 运动场大泳池
        SportGymnasium,             ///< 运动场体育馆
        SportFullstadium,           ///< 运动场满座体育场
        SportStadiumtannoy,         ///< 运动场广播系统
        PrefabWorkshop,             ///< 预制车间
        PrefabSchoolroom,           ///< 预制教室
        PrefabPractiseroom,         ///< 预制练习室
        PrefabOuthouse,             ///< 预制外屋
        PrefabCaravan,              ///< 预制拖车
        DomeTomb,                   ///< 圆顶墓穴
        PipeSmall,                  ///< 小管道
        DomeSaintpauls,             ///< 圣保罗大教堂
        PipeLongthin,               ///< 细长管道
        PipeLarge,                  ///< 大管道
        PipeResonant,               ///< 共鸣管道
        OutdoorsBackyard,           ///< 户外后院
        OutdoorsRollingplains,      ///< 户外平原
        OutdoorsDeepcanyon,         ///< 户外深峡谷
        OutdoorsCreek,              ///< 户外小溪
        OutdoorsValley,             ///< 户外山谷
        MoodHeaven,                 ///< 氛围天堂
        MoodHell,                   ///< 氛围地狱
        MoodMemory,                 ///< 氛围记忆
        DrivingCommentator,         ///< 驾驶解说员
        DrivingPitgarage,           ///< 驾驶维修站
        DrivingIncarRacer,          ///< 驾驶赛车内
        DrivingIncarSports,         ///< 驾驶跑车内
        DrivingIncarLuxury,         ///< 驾驶豪华车内
        DrivingFullgrandstand,      ///< 驾驶满座看台
        DrivingEmptygrandstand,     ///< 驾驶空看台
        DrivingTunnel,              ///< 驾驶隧道
        CityStreets,                ///< 城市街道
        CitySubway,                 ///< 城市地铁
        CityMuseum,                 ///< 城市博物馆
        CityLibrary,                ///< 城市图书馆
        CityUnderpass,              ///< 城市地下通道
        CityAbandoned,              ///< 城市废弃区
        Dustyroom,                  ///< 尘土飞扬的房间
        Chapel,                     ///< 小教堂
        Smallwaterroom,             ///< 小水房

        ENUM_CLASS_RANGE(Generic, Smallwaterroom)
    };

    /**
     * 混响预设属性
     */
    struct ReverbPresetProperties
    {
        u8char name_cn[32];          ///< 中文名称
        u8char name_en[32];          ///< 英文名称
        
        float Density;               ///< 密度
        float Diffusion;             ///< 扩散
        float Gain;                  ///< 增益
        float GainHF;                ///< 高频增益
        float GainLF;                ///< 低频增益
        float DecayTime;             ///< 衰减时间（秒）
        float DecayHFRatio;          ///< 高频衰减比
        float DecayLFRatio;          ///< 低频衰减比
        float ReflectionsGain;       ///< 初期反射增益
        float ReflectionsDelay;      ///< 初期反射延迟（秒）
        float ReflectionsPan[3];     ///< 初期反射声相（XYZ）
        float LateReverbGain;        ///< 后期混响增益
        float LateReverbDelay;       ///< 后期混响延迟（秒）
        float LateReverbPan[3];      ///< 后期混响声相（XYZ）
        float EchoTime;              ///< 回声时间（秒）
        float EchoDepth;             ///< 回声深度
        float ModulationTime;        ///< 调制时间（秒）
        float ModulationDepth;       ///< 调制深度
        float AirAbsorptionGainHF;   ///< 高频空气吸收增益
        float HFReference;           ///< 高频参考值
        float LFReference;           ///< 低频参考值
        float RoomRolloffFactor;     ///< 房间滚降因子
        int   DecayHFLimit;          ///< 高频衰减限制（布尔表示为0/1）
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
