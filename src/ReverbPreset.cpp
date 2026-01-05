#include<hgl/audio/ReverbPreset.h>
#include<hgl/al/efx-presets.h>

namespace hgl
{
    // 混响预设数据表 - 所有 113 个 OpenAL Soft 官方预设
    static const AudioReverbPresetProperties reverb_presets[] =
    {
        // Generic - 通用
        {
            u8"通用",
            u8"Generic",
            EFX_REVERB_PRESET_GENERIC
        },
        // Paddedcell - 带衬垫的单元
        {
            u8"带衬垫的单元",
            u8"Paddedcell",
            EFX_REVERB_PRESET_PADDEDCELL
        },
        // Room - 房间
        {
            u8"房间",
            u8"Room",
            EFX_REVERB_PRESET_ROOM
        },
        // Bathroom - 浴室
        {
            u8"浴室",
            u8"Bathroom",
            EFX_REVERB_PRESET_BATHROOM
        },
        // Livingroom - 客厅
        {
            u8"客厅",
            u8"Livingroom",
            EFX_REVERB_PRESET_LIVINGROOM
        },
        // Stoneroom - 石质房间
        {
            u8"石质房间",
            u8"Stoneroom",
            EFX_REVERB_PRESET_STONEROOM
        },
        // Auditorium - 礼堂
        {
            u8"礼堂",
            u8"Auditorium",
            EFX_REVERB_PRESET_AUDITORIUM
        },
        // Concerthall - 音乐厅
        {
            u8"音乐厅",
            u8"Concerthall",
            EFX_REVERB_PRESET_CONCERTHALL
        },
        // Cave - 洞穴
        {
            u8"洞穴",
            u8"Cave",
            EFX_REVERB_PRESET_CAVE
        },
        // Arena - 竞技场
        {
            u8"竞技场",
            u8"Arena",
            EFX_REVERB_PRESET_ARENA
        },
        // Hangar - 机库
        {
            u8"机库",
            u8"Hangar",
            EFX_REVERB_PRESET_HANGAR
        },
        // Carpetedhallway - 地毯走廊
        {
            u8"地毯走廊",
            u8"Carpetedhallway",
            EFX_REVERB_PRESET_CARPETEDHALLWAY
        },
        // Hallway - 走廊
        {
            u8"走廊",
            u8"Hallway",
            EFX_REVERB_PRESET_HALLWAY
        },
        // Stonecorridor - 石质走廊
        {
            u8"石质走廊",
            u8"Stonecorridor",
            EFX_REVERB_PRESET_STONECORRIDOR
        },
        // Alley - 小巷
        {
            u8"小巷",
            u8"Alley",
            EFX_REVERB_PRESET_ALLEY
        },
        // Forest - 森林
        {
            u8"森林",
            u8"Forest",
            EFX_REVERB_PRESET_FOREST
        },
        // City - 城市
        {
            u8"城市",
            u8"City",
            EFX_REVERB_PRESET_CITY
        },
        // Mountains - 山脉
        {
            u8"山脉",
            u8"Mountains",
            EFX_REVERB_PRESET_MOUNTAINS
        },
        // Quarry - 采石场
        {
            u8"采石场",
            u8"Quarry",
            EFX_REVERB_PRESET_QUARRY
        },
        // Plain - 平原
        {
            u8"平原",
            u8"Plain",
            EFX_REVERB_PRESET_PLAIN
        },
        // Parkinglot - 停车场
        {
            u8"停车场",
            u8"Parkinglot",
            EFX_REVERB_PRESET_PARKINGLOT
        },
        // Sewerpipe - 下水道
        {
            u8"下水道",
            u8"Sewerpipe",
            EFX_REVERB_PRESET_SEWERPIPE
        },
        // Underwater - 水下
        {
            u8"水下",
            u8"Underwater",
            EFX_REVERB_PRESET_UNDERWATER
        },
        // Drugged - 迷幻
        {
            u8"迷幻",
            u8"Drugged",
            EFX_REVERB_PRESET_DRUGGED
        },
        // Dizzy - 眩晕
        {
            u8"眩晕",
            u8"Dizzy",
            EFX_REVERB_PRESET_DIZZY
        },
        // Psychotic - 精神错乱
        {
            u8"精神错乱",
            u8"Psychotic",
            EFX_REVERB_PRESET_PSYCHOTIC
        },
        // CastleSmallroom - 城堡小房间
        {
            u8"城堡小房间",
            u8"Castle Smallroom",
            EFX_REVERB_PRESET_CASTLE_SMALLROOM
        },
        // CastleShortpassage - 城堡短通道
        {
            u8"城堡短通道",
            u8"Castle Shortpassage",
            EFX_REVERB_PRESET_CASTLE_SHORTPASSAGE
        },
        // CastleMediumroom - 城堡中房间
        {
            u8"城堡中房间",
            u8"Castle Mediumroom",
            EFX_REVERB_PRESET_CASTLE_MEDIUMROOM
        },
        // CastleLargeroom - 城堡大房间
        {
            u8"城堡大房间",
            u8"Castle Largeroom",
            EFX_REVERB_PRESET_CASTLE_LARGEROOM
        },
        // CastleLongpassage - 城堡长通道
        {
            u8"城堡长通道",
            u8"Castle Longpassage",
            EFX_REVERB_PRESET_CASTLE_LONGPASSAGE
        },
        // CastleHall - 城堡大厅
        {
            u8"城堡大厅",
            u8"Castle Hall",
            EFX_REVERB_PRESET_CASTLE_HALL
        },
        // CastleCupboard - 城堡橱柜
        {
            u8"城堡橱柜",
            u8"Castle Cupboard",
            EFX_REVERB_PRESET_CASTLE_CUPBOARD
        },
        // CastleCourtyard - 城堡庭院
        {
            u8"城堡庭院",
            u8"Castle Courtyard",
            EFX_REVERB_PRESET_CASTLE_COURTYARD
        },
        // CastleAlcove - 城堡壁龛
        {
            u8"城堡壁龛",
            u8"Castle Alcove",
            EFX_REVERB_PRESET_CASTLE_ALCOVE
        },
        // FactorySmallroom - 工厂小房间
        {
            u8"工厂小房间",
            u8"Factory Smallroom",
            EFX_REVERB_PRESET_FACTORY_SMALLROOM
        },
        // FactoryShortpassage - 工厂短通道
        {
            u8"工厂短通道",
            u8"Factory Shortpassage",
            EFX_REVERB_PRESET_FACTORY_SHORTPASSAGE
        },
        // FactoryMediumroom - 工厂中房间
        {
            u8"工厂中房间",
            u8"Factory Mediumroom",
            EFX_REVERB_PRESET_FACTORY_MEDIUMROOM
        },
        // FactoryLargeroom - 工厂大房间
        {
            u8"工厂大房间",
            u8"Factory Largeroom",
            EFX_REVERB_PRESET_FACTORY_LARGEROOM
        },
        // FactoryLongpassage - 工厂长通道
        {
            u8"工厂长通道",
            u8"Factory Longpassage",
            EFX_REVERB_PRESET_FACTORY_LONGPASSAGE
        },
        // FactoryHall - 工厂大厅
        {
            u8"工厂大厅",
            u8"Factory Hall",
            EFX_REVERB_PRESET_FACTORY_HALL
        },
        // FactoryCupboard - 工厂橱柜
        {
            u8"工厂橱柜",
            u8"Factory Cupboard",
            EFX_REVERB_PRESET_FACTORY_CUPBOARD
        },
        // FactoryCourtyard - 工厂庭院
        {
            u8"工厂庭院",
            u8"Factory Courtyard",
            EFX_REVERB_PRESET_FACTORY_COURTYARD
        },
        // FactoryAlcove - 工厂壁龛
        {
            u8"工厂壁龛",
            u8"Factory Alcove",
            EFX_REVERB_PRESET_FACTORY_ALCOVE
        },
        // IcepalaceSmallroom - 冰宫小房间
        {
            u8"冰宫小房间",
            u8"Icepalace Smallroom",
            EFX_REVERB_PRESET_ICEPALACE_SMALLROOM
        },
        // IcepalaceShortpassage - 冰宫短通道
        {
            u8"冰宫短通道",
            u8"Icepalace Shortpassage",
            EFX_REVERB_PRESET_ICEPALACE_SHORTPASSAGE
        },
        // IcepalaceMediumroom - 冰宫中房间
        {
            u8"冰宫中房间",
            u8"Icepalace Mediumroom",
            EFX_REVERB_PRESET_ICEPALACE_MEDIUMROOM
        },
        // IcepalaceLargeroom - 冰宫大房间
        {
            u8"冰宫大房间",
            u8"Icepalace Largeroom",
            EFX_REVERB_PRESET_ICEPALACE_LARGEROOM
        },
        // IcepalaceLongpassage - 冰宫长通道
        {
            u8"冰宫长通道",
            u8"Icepalace Longpassage",
            EFX_REVERB_PRESET_ICEPALACE_LONGPASSAGE
        },
        // IcepalaceHall - 冰宫大厅
        {
            u8"冰宫大厅",
            u8"Icepalace Hall",
            EFX_REVERB_PRESET_ICEPALACE_HALL
        },
        // IcepalaceCupboard - 冰宫橱柜
        {
            u8"冰宫橱柜",
            u8"Icepalace Cupboard",
            EFX_REVERB_PRESET_ICEPALACE_CUPBOARD
        },
        // IcepalaceCourtyard - 冰宫庭院
        {
            u8"冰宫庭院",
            u8"Icepalace Courtyard",
            EFX_REVERB_PRESET_ICEPALACE_COURTYARD
        },
        // IcepalaceAlcove - 冰宫壁龛
        {
            u8"冰宫壁龛",
            u8"Icepalace Alcove",
            EFX_REVERB_PRESET_ICEPALACE_ALCOVE
        },
        // SpacestationSmallroom - 空间站小房间
        {
            u8"空间站小房间",
            u8"Spacestation Smallroom",
            EFX_REVERB_PRESET_SPACESTATION_SMALLROOM
        },
        // SpacestationShortpassage - 空间站短通道
        {
            u8"空间站短通道",
            u8"Spacestation Shortpassage",
            EFX_REVERB_PRESET_SPACESTATION_SHORTPASSAGE
        },
        // SpacestationMediumroom - 空间站中房间
        {
            u8"空间站中房间",
            u8"Spacestation Mediumroom",
            EFX_REVERB_PRESET_SPACESTATION_MEDIUMROOM
        },
        // SpacestationLargeroom - 空间站大房间
        {
            u8"空间站大房间",
            u8"Spacestation Largeroom",
            EFX_REVERB_PRESET_SPACESTATION_LARGEROOM
        },
        // SpacestationLongpassage - 空间站长通道
        {
            u8"空间站长通道",
            u8"Spacestation Longpassage",
            EFX_REVERB_PRESET_SPACESTATION_LONGPASSAGE
        },
        // SpacestationHall - 空间站大厅
        {
            u8"空间站大厅",
            u8"Spacestation Hall",
            EFX_REVERB_PRESET_SPACESTATION_HALL
        },
        // SpacestationCupboard - 空间站橱柜
        {
            u8"空间站橱柜",
            u8"Spacestation Cupboard",
            EFX_REVERB_PRESET_SPACESTATION_CUPBOARD
        },
        // SpacestationAlcove - 空间站壁龛
        {
            u8"空间站壁龛",
            u8"Spacestation Alcove",
            EFX_REVERB_PRESET_SPACESTATION_ALCOVE
        },
        // WoodenSmallroom - 木质小房间
        {
            u8"木质小房间",
            u8"Wooden Smallroom",
            EFX_REVERB_PRESET_WOODEN_SMALLROOM
        },
        // WoodenShortpassage - 木质短通道
        {
            u8"木质短通道",
            u8"Wooden Shortpassage",
            EFX_REVERB_PRESET_WOODEN_SHORTPASSAGE
        },
        // WoodenMediumroom - 木质中房间
        {
            u8"木质中房间",
            u8"Wooden Mediumroom",
            EFX_REVERB_PRESET_WOODEN_MEDIUMROOM
        },
        // WoodenLargeroom - 木质大房间
        {
            u8"木质大房间",
            u8"Wooden Largeroom",
            EFX_REVERB_PRESET_WOODEN_LARGEROOM
        },
        // WoodenLongpassage - 木质长通道
        {
            u8"木质长通道",
            u8"Wooden Longpassage",
            EFX_REVERB_PRESET_WOODEN_LONGPASSAGE
        },
        // WoodenHall - 木质大厅
        {
            u8"木质大厅",
            u8"Wooden Hall",
            EFX_REVERB_PRESET_WOODEN_HALL
        },
        // WoodenCupboard - 木质橱柜
        {
            u8"木质橱柜",
            u8"Wooden Cupboard",
            EFX_REVERB_PRESET_WOODEN_CUPBOARD
        },
        // WoodenCourtyard - 木质庭院
        {
            u8"木质庭院",
            u8"Wooden Courtyard",
            EFX_REVERB_PRESET_WOODEN_COURTYARD
        },
        // WoodenAlcove - 木质壁龛
        {
            u8"木质壁龛",
            u8"Wooden Alcove",
            EFX_REVERB_PRESET_WOODEN_ALCOVE
        },
        // SportEmptystadium - 运动场空体育场
        {
            u8"运动场空体育场",
            u8"Sport Emptystadium",
            EFX_REVERB_PRESET_SPORT_EMPTYSTADIUM
        },
        // SportSquashcourt - 运动场壁球场
        {
            u8"运动场壁球场",
            u8"Sport Squashcourt",
            EFX_REVERB_PRESET_SPORT_SQUASHCOURT
        },
        // SportSmallswimmingpool - 运动场小泳池
        {
            u8"运动场小泳池",
            u8"Sport Smallswimmingpool",
            EFX_REVERB_PRESET_SPORT_SMALLSWIMMINGPOOL
        },
        // SportLargeswimmingpool - 运动场大泳池
        {
            u8"运动场大泳池",
            u8"Sport Largeswimmingpool",
            EFX_REVERB_PRESET_SPORT_LARGESWIMMINGPOOL
        },
        // SportGymnasium - 运动场体育馆
        {
            u8"运动场体育馆",
            u8"Sport Gymnasium",
            EFX_REVERB_PRESET_SPORT_GYMNASIUM
        },
        // SportFullstadium - 运动场满座体育场
        {
            u8"运动场满座体育场",
            u8"Sport Fullstadium",
            EFX_REVERB_PRESET_SPORT_FULLSTADIUM
        },
        // SportStadiumtannoy - 运动场广播系统
        {
            u8"运动场广播系统",
            u8"Sport Stadiumtannoy",
            EFX_REVERB_PRESET_SPORT_STADIUMTANNOY
        },
        // PrefabWorkshop - 预制车间
        {
            u8"预制车间",
            u8"Prefab Workshop",
            EFX_REVERB_PRESET_PREFAB_WORKSHOP
        },
        // PrefabSchoolroom - 预制教室
        {
            u8"预制教室",
            u8"Prefab Schoolroom",
            EFX_REVERB_PRESET_PREFAB_SCHOOLROOM
        },
        // PrefabPractiseroom - 预制练习室
        {
            u8"预制练习室",
            u8"Prefab Practiseroom",
            EFX_REVERB_PRESET_PREFAB_PRACTISEROOM
        },
        // PrefabOuthouse - 预制外屋
        {
            u8"预制外屋",
            u8"Prefab Outhouse",
            EFX_REVERB_PRESET_PREFAB_OUTHOUSE
        },
        // PrefabCaravan - 预制拖车
        {
            u8"预制拖车",
            u8"Prefab Caravan",
            EFX_REVERB_PRESET_PREFAB_CARAVAN
        },
        // DomeTomb - 圆顶墓穴
        {
            u8"圆顶墓穴",
            u8"Dome Tomb",
            EFX_REVERB_PRESET_DOME_TOMB
        },
        // PipeSmall - 小管道
        {
            u8"小管道",
            u8"Pipe Small",
            EFX_REVERB_PRESET_PIPE_SMALL
        },
        // DomeSaintpauls - 圣保罗大教堂
        {
            u8"圣保罗大教堂",
            u8"Dome Saintpauls",
            EFX_REVERB_PRESET_DOME_SAINTPAULS
        },
        // PipeLongthin - 细长管道
        {
            u8"细长管道",
            u8"Pipe Longthin",
            EFX_REVERB_PRESET_PIPE_LONGTHIN
        },
        // PipeLarge - 大管道
        {
            u8"大管道",
            u8"Pipe Large",
            EFX_REVERB_PRESET_PIPE_LARGE
        },
        // PipeResonant - 共鸣管道
        {
            u8"共鸣管道",
            u8"Pipe Resonant",
            EFX_REVERB_PRESET_PIPE_RESONANT
        },
        // OutdoorsBackyard - 户外后院
        {
            u8"户外后院",
            u8"Outdoors Backyard",
            EFX_REVERB_PRESET_OUTDOORS_BACKYARD
        },
        // OutdoorsRollingplains - 户外平原
        {
            u8"户外平原",
            u8"Outdoors Rollingplains",
            EFX_REVERB_PRESET_OUTDOORS_ROLLINGPLAINS
        },
        // OutdoorsDeepcanyon - 户外深峡谷
        {
            u8"户外深峡谷",
            u8"Outdoors Deepcanyon",
            EFX_REVERB_PRESET_OUTDOORS_DEEPCANYON
        },
        // OutdoorsCreek - 户外小溪
        {
            u8"户外小溪",
            u8"Outdoors Creek",
            EFX_REVERB_PRESET_OUTDOORS_CREEK
        },
        // OutdoorsValley - 户外山谷
        {
            u8"户外山谷",
            u8"Outdoors Valley",
            EFX_REVERB_PRESET_OUTDOORS_VALLEY
        },
        // MoodHeaven - 氛围天堂
        {
            u8"氛围天堂",
            u8"Mood Heaven",
            EFX_REVERB_PRESET_MOOD_HEAVEN
        },
        // MoodHell - 氛围地狱
        {
            u8"氛围地狱",
            u8"Mood Hell",
            EFX_REVERB_PRESET_MOOD_HELL
        },
        // MoodMemory - 氛围记忆
        {
            u8"氛围记忆",
            u8"Mood Memory",
            EFX_REVERB_PRESET_MOOD_MEMORY
        },
        // DrivingCommentator - 驾驶解说员
        {
            u8"驾驶解说员",
            u8"Driving Commentator",
            EFX_REVERB_PRESET_DRIVING_COMMENTATOR
        },
        // DrivingPitgarage - 驾驶维修站
        {
            u8"驾驶维修站",
            u8"Driving Pitgarage",
            EFX_REVERB_PRESET_DRIVING_PITGARAGE
        },
        // DrivingIncarRacer - 驾驶赛车内
        {
            u8"驾驶赛车内",
            u8"Driving Incar Racer",
            EFX_REVERB_PRESET_DRIVING_INCAR_RACER
        },
        // DrivingIncarSports - 驾驶跑车内
        {
            u8"驾驶跑车内",
            u8"Driving Incar Sports",
            EFX_REVERB_PRESET_DRIVING_INCAR_SPORTS
        },
        // DrivingIncarLuxury - 驾驶豪华车内
        {
            u8"驾驶豪华车内",
            u8"Driving Incar Luxury",
            EFX_REVERB_PRESET_DRIVING_INCAR_LUXURY
        },
        // DrivingFullgrandstand - 驾驶满座看台
        {
            u8"驾驶满座看台",
            u8"Driving Fullgrandstand",
            EFX_REVERB_PRESET_DRIVING_FULLGRANDSTAND
        },
        // DrivingEmptygrandstand - 驾驶空看台
        {
            u8"驾驶空看台",
            u8"Driving Emptygrandstand",
            EFX_REVERB_PRESET_DRIVING_EMPTYGRANDSTAND
        },
        // DrivingTunnel - 驾驶隧道
        {
            u8"驾驶隧道",
            u8"Driving Tunnel",
            EFX_REVERB_PRESET_DRIVING_TUNNEL
        },
        // CityStreets - 城市街道
        {
            u8"城市街道",
            u8"City Streets",
            EFX_REVERB_PRESET_CITY_STREETS
        },
        // CitySubway - 城市地铁
        {
            u8"城市地铁",
            u8"City Subway",
            EFX_REVERB_PRESET_CITY_SUBWAY
        },
        // CityMuseum - 城市博物馆
        {
            u8"城市博物馆",
            u8"City Museum",
            EFX_REVERB_PRESET_CITY_MUSEUM
        },
        // CityLibrary - 城市图书馆
        {
            u8"城市图书馆",
            u8"City Library",
            EFX_REVERB_PRESET_CITY_LIBRARY
        },
        // CityUnderpass - 城市地下通道
        {
            u8"城市地下通道",
            u8"City Underpass",
            EFX_REVERB_PRESET_CITY_UNDERPASS
        },
        // CityAbandoned - 城市废弃区
        {
            u8"城市废弃区",
            u8"City Abandoned",
            EFX_REVERB_PRESET_CITY_ABANDONED
        },
        // Dustyroom - 尘土飞扬的房间
        {
            u8"尘土飞扬的房间",
            u8"Dustyroom",
            EFX_REVERB_PRESET_DUSTYROOM
        },
        // Chapel - 小教堂
        {
            u8"小教堂",
            u8"Chapel",
            EFX_REVERB_PRESET_CHAPEL
        },
        // Smallwaterroom - 小水房
        {
            u8"小水房",
            u8"Smallwaterroom",
            EFX_REVERB_PRESET_SMALLWATERROOM
        }
    };

    const AudioReverbPresetProperties *GetAudioReverbPresetProperties(AudioReverbPreset preset)
    {
        int index = static_cast<int>(preset);
        if (index < 0 || index >= GetAudioReverbPresetCount())
            return nullptr;
        return &reverb_presets[index];
    }

    int GetAudioReverbPresetCount()
    {
        return sizeof(reverb_presets) / sizeof(reverb_presets[0]);
    }
}//namespace hgl
