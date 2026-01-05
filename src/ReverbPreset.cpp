#include<hgl/audio/ReverbPreset.h>

namespace hgl
{
    // 混响预设数据表 - 所有 113 个 OpenAL Soft 官方预设
    static const ReverbPresetProperties reverb_presets[] = {
        // Generic - 通用
        {
            UTF8String(u8"通用"),
            UTF8String(u8"Generic"),
            1.0000f, 1.0000f, 0.3162f, 0.8913f, 1.0000f, 1.4900f, 0.8300f, 1.0000f, 0.0500f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Paddedcell - 带衬垫的单元
        {
            UTF8String(u8"带衬垫的单元"),
            UTF8String(u8"Paddedcell"),
            0.1715f, 1.0000f, 0.3162f, 0.0010f, 1.0000f, 0.1700f, 0.1000f, 1.0000f, 0.2500f, 0.0010f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Room - 房间
        {
            UTF8String(u8"房间"),
            UTF8String(u8"Room"),
            0.4287f, 1.0000f, 0.3162f, 0.5929f, 1.0000f, 0.4000f, 0.8300f, 1.0000f, 0.1503f, 0.0020f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Bathroom - 浴室
        {
            UTF8String(u8"浴室"),
            UTF8String(u8"Bathroom"),
            0.1715f, 1.0000f, 0.3162f, 0.2512f, 1.0000f, 1.4900f, 0.5400f, 1.0000f, 0.6531f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Livingroom - 客厅
        {
            UTF8String(u8"客厅"),
            UTF8String(u8"Livingroom"),
            0.9766f, 1.0000f, 0.3162f, 0.0010f, 1.0000f, 0.5000f, 0.1000f, 1.0000f, 0.2051f, 0.0030f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Stoneroom - 石质房间
        {
            UTF8String(u8"石质房间"),
            UTF8String(u8"Stoneroom"),
            1.0000f, 1.0000f, 0.3162f, 0.7079f, 1.0000f, 2.3100f, 0.6400f, 1.0000f, 0.4411f, 0.0120f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Auditorium - 礼堂
        {
            UTF8String(u8"礼堂"),
            UTF8String(u8"Auditorium"),
            1.0000f, 1.0000f, 0.3162f, 0.5781f, 1.0000f, 4.3200f, 0.5900f, 1.0000f, 0.4032f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Concerthall - 音乐厅
        {
            UTF8String(u8"音乐厅"),
            UTF8String(u8"Concerthall"),
            1.0000f, 1.0000f, 0.3162f, 0.5623f, 1.0000f, 3.9200f, 0.7000f, 1.0000f, 0.2427f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Cave - 洞穴
        {
            UTF8String(u8"洞穴"),
            UTF8String(u8"Cave"),
            1.0000f, 1.0000f, 0.3162f, 1.0000f, 1.0000f, 2.9100f, 1.3000f, 1.0000f, 0.5000f, 0.0150f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Arena - 竞技场
        {
            UTF8String(u8"竞技场"),
            UTF8String(u8"Arena"),
            1.0000f, 1.0000f, 0.3162f, 0.4477f, 1.0000f, 7.2400f, 0.3300f, 1.0000f, 0.2612f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Hangar - 机库
        {
            UTF8String(u8"机库"),
            UTF8String(u8"Hangar"),
            1.0000f, 1.0000f, 0.3162f, 0.3162f, 1.0000f, 10.0500f, 0.2300f, 1.0000f, 0.5000f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Carpetedhallway - 地毯走廊
        {
            UTF8String(u8"地毯走廊"),
            UTF8String(u8"Carpetedhallway"),
            0.4287f, 1.0000f, 0.3162f, 0.0100f, 1.0000f, 0.3000f, 0.1000f, 1.0000f, 0.1215f, 0.0020f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Hallway - 走廊
        {
            UTF8String(u8"走廊"),
            UTF8String(u8"Hallway"),
            0.3645f, 1.0000f, 0.3162f, 0.7079f, 1.0000f, 1.4900f, 0.5900f, 1.0000f, 0.2458f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Stonecorridor - 石质走廊
        {
            UTF8String(u8"石质走廊"),
            UTF8String(u8"Stonecorridor"),
            1.0000f, 1.0000f, 0.3162f, 0.7612f, 1.0000f, 2.7000f, 0.7900f, 1.0000f, 0.2472f, 0.0130f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Alley - 小巷
        {
            UTF8String(u8"小巷"),
            UTF8String(u8"Alley"),
            1.0000f, 0.3000f, 0.3162f, 0.7328f, 1.0000f, 1.4900f, 0.8600f, 1.0000f, 0.2500f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Forest - 森林
        {
            UTF8String(u8"森林"),
            UTF8String(u8"Forest"),
            1.0000f, 0.3000f, 0.3162f, 0.0224f, 1.0000f, 1.4900f, 0.5400f, 1.0000f, 0.0525f, 0.1620f, { 0.0000f, 0.0000f, 0.0000f
        },
        // City - 城市
        {
            UTF8String(u8"城市"),
            UTF8String(u8"City"),
            1.0000f, 0.5000f, 0.3162f, 0.3981f, 1.0000f, 1.4900f, 0.6700f, 1.0000f, 0.0730f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Mountains - 山脉
        {
            UTF8String(u8"山脉"),
            UTF8String(u8"Mountains"),
            1.0000f, 0.2700f, 0.3162f, 0.0562f, 1.0000f, 1.4900f, 0.2100f, 1.0000f, 0.0407f, 0.3000f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Quarry - 采石场
        {
            UTF8String(u8"采石场"),
            UTF8String(u8"Quarry"),
            1.0000f, 1.0000f, 0.3162f, 0.3162f, 1.0000f, 1.4900f, 0.8300f, 1.0000f, 0.0000f, 0.0610f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Plain - 平原
        {
            UTF8String(u8"平原"),
            UTF8String(u8"Plain"),
            1.0000f, 0.2100f, 0.3162f, 0.1000f, 1.0000f, 1.4900f, 0.5000f, 1.0000f, 0.0585f, 0.1790f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Parkinglot - 停车场
        {
            UTF8String(u8"停车场"),
            UTF8String(u8"Parkinglot"),
            1.0000f, 1.0000f, 0.3162f, 1.0000f, 1.0000f, 1.6500f, 1.5000f, 1.0000f, 0.2082f, 0.0080f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Sewerpipe - 下水道
        {
            UTF8String(u8"下水道"),
            UTF8String(u8"Sewerpipe"),
            0.3071f, 0.8000f, 0.3162f, 0.3162f, 1.0000f, 2.8100f, 0.1400f, 1.0000f, 1.6387f, 0.0140f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Underwater - 水下
        {
            UTF8String(u8"水下"),
            UTF8String(u8"Underwater"),
            0.3645f, 1.0000f, 0.3162f, 0.0100f, 1.0000f, 1.4900f, 0.1000f, 1.0000f, 0.5963f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Drugged - 迷幻
        {
            UTF8String(u8"迷幻"),
            UTF8String(u8"Drugged"),
            0.4287f, 0.5000f, 0.3162f, 1.0000f, 1.0000f, 8.3900f, 1.3900f, 1.0000f, 0.8760f, 0.0020f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Dizzy - 眩晕
        {
            UTF8String(u8"眩晕"),
            UTF8String(u8"Dizzy"),
            0.3645f, 0.6000f, 0.3162f, 0.6310f, 1.0000f, 17.2300f, 0.5600f, 1.0000f, 0.1392f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Psychotic - 精神错乱
        {
            UTF8String(u8"精神错乱"),
            UTF8String(u8"Psychotic"),
            0.0625f, 0.5000f, 0.3162f, 0.8404f, 1.0000f, 7.5600f, 0.9100f, 1.0000f, 0.4864f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Castle Smallroom - 城堡小房间
        {
            UTF8String(u8"城堡小房间"),
            UTF8String(u8"Castle Smallroom"),
            1.0000f, 0.8900f, 0.3162f, 0.3981f, 0.1000f, 1.2200f, 0.8300f, 0.3100f, 0.8913f, 0.0220f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Castle Shortpassage - 城堡短通道
        {
            UTF8String(u8"城堡短通道"),
            UTF8String(u8"Castle Shortpassage"),
            1.0000f, 0.8900f, 0.3162f, 0.3162f, 0.1000f, 2.3200f, 0.8300f, 0.3100f, 0.8913f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Castle Mediumroom - 城堡中房间
        {
            UTF8String(u8"城堡中房间"),
            UTF8String(u8"Castle Mediumroom"),
            1.0000f, 0.9300f, 0.3162f, 0.2818f, 0.1000f, 2.0400f, 0.8300f, 0.4600f, 0.6310f, 0.0220f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Castle Largeroom - 城堡大房间
        {
            UTF8String(u8"城堡大房间"),
            UTF8String(u8"Castle Largeroom"),
            1.0000f, 0.8200f, 0.3162f, 0.2818f, 0.1259f, 2.5300f, 0.8300f, 0.5000f, 0.4467f, 0.0340f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Castle Longpassage - 城堡长通道
        {
            UTF8String(u8"城堡长通道"),
            UTF8String(u8"Castle Longpassage"),
            1.0000f, 0.8900f, 0.3162f, 0.3981f, 0.1000f, 3.4200f, 0.8300f, 0.3100f, 0.8913f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Castle Hall - 城堡大厅
        {
            UTF8String(u8"城堡大厅"),
            UTF8String(u8"Castle Hall"),
            1.0000f, 0.8100f, 0.3162f, 0.2818f, 0.1778f, 3.1400f, 0.7900f, 0.6200f, 0.1778f, 0.0560f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Castle Cupboard - 城堡橱柜
        {
            UTF8String(u8"城堡橱柜"),
            UTF8String(u8"Castle Cupboard"),
            1.0000f, 0.8900f, 0.3162f, 0.2818f, 0.1000f, 0.6700f, 0.8700f, 0.3100f, 1.4125f, 0.0100f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Castle Courtyard - 城堡庭院
        {
            UTF8String(u8"城堡庭院"),
            UTF8String(u8"Castle Courtyard"),
            1.0000f, 0.4200f, 0.3162f, 0.4467f, 0.1995f, 2.1300f, 0.6100f, 0.2300f, 0.2239f, 0.1600f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Castle Alcove - 城堡壁龛
        {
            UTF8String(u8"城堡壁龛"),
            UTF8String(u8"Castle Alcove"),
            1.0000f, 0.8900f, 0.3162f, 0.5012f, 0.1000f, 1.6400f, 0.8700f, 0.3100f, 1.0000f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Factory Smallroom - 工厂小房间
        {
            UTF8String(u8"工厂小房间"),
            UTF8String(u8"Factory Smallroom"),
            0.3645f, 0.8200f, 0.3162f, 0.7943f, 0.5012f, 1.7200f, 0.6500f, 1.3100f, 0.7079f, 0.0100f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Factory Shortpassage - 工厂短通道
        {
            UTF8String(u8"工厂短通道"),
            UTF8String(u8"Factory Shortpassage"),
            0.3645f, 0.6400f, 0.2512f, 0.7943f, 0.5012f, 2.5300f, 0.6500f, 1.3100f, 1.0000f, 0.0100f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Factory Mediumroom - 工厂中房间
        {
            UTF8String(u8"工厂中房间"),
            UTF8String(u8"Factory Mediumroom"),
            0.4287f, 0.8200f, 0.2512f, 0.7943f, 0.5012f, 2.7600f, 0.6500f, 1.3100f, 0.2818f, 0.0220f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Factory Largeroom - 工厂大房间
        {
            UTF8String(u8"工厂大房间"),
            UTF8String(u8"Factory Largeroom"),
            0.4287f, 0.7500f, 0.2512f, 0.7079f, 0.6310f, 4.2400f, 0.5100f, 1.3100f, 0.1778f, 0.0390f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Factory Longpassage - 工厂长通道
        {
            UTF8String(u8"工厂长通道"),
            UTF8String(u8"Factory Longpassage"),
            0.3645f, 0.6400f, 0.2512f, 0.7943f, 0.5012f, 4.0600f, 0.6500f, 1.3100f, 1.0000f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Factory Hall - 工厂大厅
        {
            UTF8String(u8"工厂大厅"),
            UTF8String(u8"Factory Hall"),
            0.4287f, 0.7500f, 0.3162f, 0.7079f, 0.6310f, 7.4300f, 0.5100f, 1.3100f, 0.0631f, 0.0730f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Factory Cupboard - 工厂橱柜
        {
            UTF8String(u8"工厂橱柜"),
            UTF8String(u8"Factory Cupboard"),
            0.3071f, 0.6300f, 0.2512f, 0.7943f, 0.5012f, 0.4900f, 0.6500f, 1.3100f, 1.2589f, 0.0100f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Factory Courtyard - 工厂庭院
        {
            UTF8String(u8"工厂庭院"),
            UTF8String(u8"Factory Courtyard"),
            0.3071f, 0.5700f, 0.3162f, 0.3162f, 0.6310f, 2.3200f, 0.2900f, 0.5600f, 0.2239f, 0.1400f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Factory Alcove - 工厂壁龛
        {
            UTF8String(u8"工厂壁龛"),
            UTF8String(u8"Factory Alcove"),
            0.3645f, 0.5900f, 0.2512f, 0.7943f, 0.5012f, 3.1400f, 0.6500f, 1.3100f, 1.4125f, 0.0100f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Icepalace Smallroom - 冰宫小房间
        {
            UTF8String(u8"冰宫小房间"),
            UTF8String(u8"Icepalace Smallroom"),
            1.0000f, 0.8400f, 0.3162f, 0.5623f, 0.2818f, 1.5100f, 1.5300f, 0.2700f, 0.8913f, 0.0100f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Icepalace Shortpassage - 冰宫短通道
        {
            UTF8String(u8"冰宫短通道"),
            UTF8String(u8"Icepalace Shortpassage"),
            1.0000f, 0.7500f, 0.3162f, 0.5623f, 0.2818f, 1.7900f, 1.4600f, 0.2800f, 0.5012f, 0.0100f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Icepalace Mediumroom - 冰宫中房间
        {
            UTF8String(u8"冰宫中房间"),
            UTF8String(u8"Icepalace Mediumroom"),
            1.0000f, 0.8700f, 0.3162f, 0.5623f, 0.4467f, 2.2200f, 1.5300f, 0.3200f, 0.3981f, 0.0390f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Icepalace Largeroom - 冰宫大房间
        {
            UTF8String(u8"冰宫大房间"),
            UTF8String(u8"Icepalace Largeroom"),
            1.0000f, 0.8100f, 0.3162f, 0.5623f, 0.4467f, 3.1400f, 1.5300f, 0.3200f, 0.2512f, 0.0390f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Icepalace Longpassage - 冰宫长通道
        {
            UTF8String(u8"冰宫长通道"),
            UTF8String(u8"Icepalace Longpassage"),
            1.0000f, 0.7700f, 0.3162f, 0.5623f, 0.3981f, 3.0100f, 1.4600f, 0.2800f, 0.7943f, 0.0120f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Icepalace Hall - 冰宫大厅
        {
            UTF8String(u8"冰宫大厅"),
            UTF8String(u8"Icepalace Hall"),
            1.0000f, 0.7600f, 0.3162f, 0.4467f, 0.5623f, 5.4900f, 1.5300f, 0.3800f, 0.1122f, 0.0540f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Icepalace Cupboard - 冰宫橱柜
        {
            UTF8String(u8"冰宫橱柜"),
            UTF8String(u8"Icepalace Cupboard"),
            1.0000f, 0.8300f, 0.3162f, 0.5012f, 0.2239f, 0.7600f, 1.5300f, 0.2600f, 1.1220f, 0.0120f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Icepalace Courtyard - 冰宫庭院
        {
            UTF8String(u8"冰宫庭院"),
            UTF8String(u8"Icepalace Courtyard"),
            1.0000f, 0.5900f, 0.3162f, 0.2818f, 0.3162f, 2.0400f, 1.2000f, 0.3800f, 0.3162f, 0.1730f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Icepalace Alcove - 冰宫壁龛
        {
            UTF8String(u8"冰宫壁龛"),
            UTF8String(u8"Icepalace Alcove"),
            1.0000f, 0.8400f, 0.3162f, 0.5623f, 0.2818f, 2.7600f, 1.4600f, 0.2800f, 1.1220f, 0.0100f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Spacestation Smallroom - 空间站小房间
        {
            UTF8String(u8"空间站小房间"),
            UTF8String(u8"Spacestation Smallroom"),
            0.2109f, 0.7000f, 0.3162f, 0.7079f, 0.8913f, 1.7200f, 0.8200f, 0.5500f, 0.7943f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Spacestation Shortpassage - 空间站短通道
        {
            UTF8String(u8"空间站短通道"),
            UTF8String(u8"Spacestation Shortpassage"),
            0.2109f, 0.8700f, 0.3162f, 0.6310f, 0.8913f, 3.5700f, 0.5000f, 0.5500f, 1.0000f, 0.0120f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Spacestation Mediumroom - 空间站中房间
        {
            UTF8String(u8"空间站中房间"),
            UTF8String(u8"Spacestation Mediumroom"),
            0.2109f, 0.7500f, 0.3162f, 0.6310f, 0.8913f, 3.0100f, 0.5000f, 0.5500f, 0.3981f, 0.0340f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Spacestation Largeroom - 空间站大房间
        {
            UTF8String(u8"空间站大房间"),
            UTF8String(u8"Spacestation Largeroom"),
            0.3645f, 0.8100f, 0.3162f, 0.6310f, 0.8913f, 3.8900f, 0.3800f, 0.6100f, 0.3162f, 0.0560f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Spacestation Longpassage - 空间站长通道
        {
            UTF8String(u8"空间站长通道"),
            UTF8String(u8"Spacestation Longpassage"),
            0.4287f, 0.8200f, 0.3162f, 0.6310f, 0.8913f, 4.6200f, 0.6200f, 0.5500f, 1.0000f, 0.0120f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Spacestation Hall - 空间站大厅
        {
            UTF8String(u8"空间站大厅"),
            UTF8String(u8"Spacestation Hall"),
            0.4287f, 0.8700f, 0.3162f, 0.6310f, 0.8913f, 7.1100f, 0.3800f, 0.6100f, 0.1778f, 0.1000f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Spacestation Cupboard - 空间站橱柜
        {
            UTF8String(u8"空间站橱柜"),
            UTF8String(u8"Spacestation Cupboard"),
            0.1715f, 0.5600f, 0.3162f, 0.7079f, 0.8913f, 0.7900f, 0.8100f, 0.5500f, 1.4125f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Spacestation Alcove - 空间站壁龛
        {
            UTF8String(u8"空间站壁龛"),
            UTF8String(u8"Spacestation Alcove"),
            0.2109f, 0.7800f, 0.3162f, 0.7079f, 0.8913f, 1.1600f, 0.8100f, 0.5500f, 1.4125f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Wooden Smallroom - 木质小房间
        {
            UTF8String(u8"木质小房间"),
            UTF8String(u8"Wooden Smallroom"),
            1.0000f, 1.0000f, 0.3162f, 0.1122f, 0.3162f, 0.7900f, 0.3200f, 0.8700f, 1.0000f, 0.0320f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Wooden Shortpassage - 木质短通道
        {
            UTF8String(u8"木质短通道"),
            UTF8String(u8"Wooden Shortpassage"),
            1.0000f, 1.0000f, 0.3162f, 0.1259f, 0.3162f, 1.7500f, 0.5000f, 0.8700f, 0.8913f, 0.0120f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Wooden Mediumroom - 木质中房间
        {
            UTF8String(u8"木质中房间"),
            UTF8String(u8"Wooden Mediumroom"),
            1.0000f, 1.0000f, 0.3162f, 0.1000f, 0.2818f, 1.4700f, 0.4200f, 0.8200f, 0.8913f, 0.0490f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Wooden Largeroom - 木质大房间
        {
            UTF8String(u8"木质大房间"),
            UTF8String(u8"Wooden Largeroom"),
            1.0000f, 1.0000f, 0.3162f, 0.0891f, 0.2818f, 2.6500f, 0.3300f, 0.8200f, 0.8913f, 0.0660f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Wooden Longpassage - 木质长通道
        {
            UTF8String(u8"木质长通道"),
            UTF8String(u8"Wooden Longpassage"),
            1.0000f, 1.0000f, 0.3162f, 0.1000f, 0.3162f, 1.9900f, 0.4000f, 0.7900f, 1.0000f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Wooden Hall - 木质大厅
        {
            UTF8String(u8"木质大厅"),
            UTF8String(u8"Wooden Hall"),
            1.0000f, 1.0000f, 0.3162f, 0.0794f, 0.2818f, 3.4500f, 0.3000f, 0.8200f, 0.8913f, 0.0880f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Wooden Cupboard - 木质橱柜
        {
            UTF8String(u8"木质橱柜"),
            UTF8String(u8"Wooden Cupboard"),
            1.0000f, 1.0000f, 0.3162f, 0.1413f, 0.3162f, 0.5600f, 0.4600f, 0.9100f, 1.1220f, 0.0120f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Wooden Courtyard - 木质庭院
        {
            UTF8String(u8"木质庭院"),
            UTF8String(u8"Wooden Courtyard"),
            1.0000f, 0.6500f, 0.3162f, 0.0794f, 0.3162f, 1.7900f, 0.3500f, 0.7900f, 0.5623f, 0.1230f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Wooden Alcove - 木质壁龛
        {
            UTF8String(u8"木质壁龛"),
            UTF8String(u8"Wooden Alcove"),
            1.0000f, 1.0000f, 0.3162f, 0.1259f, 0.3162f, 1.2200f, 0.6200f, 0.9100f, 1.1220f, 0.0120f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Sport Emptystadium - 运动场空体育场
        {
            UTF8String(u8"运动场空体育场"),
            UTF8String(u8"Sport Emptystadium"),
            1.0000f, 1.0000f, 0.3162f, 0.4467f, 0.7943f, 6.2600f, 0.5100f, 1.1000f, 0.0631f, 0.1830f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Sport Squashcourt - 运动场壁球场
        {
            UTF8String(u8"运动场壁球场"),
            UTF8String(u8"Sport Squashcourt"),
            1.0000f, 0.7500f, 0.3162f, 0.3162f, 0.7943f, 2.2200f, 0.9100f, 1.1600f, 0.4467f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Sport Smallswimmingpool - 运动场小泳池
        {
            UTF8String(u8"运动场小泳池"),
            UTF8String(u8"Sport Smallswimmingpool"),
            1.0000f, 0.7000f, 0.3162f, 0.7943f, 0.8913f, 2.7600f, 1.2500f, 1.1400f, 0.6310f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Sport Largeswimmingpool - 运动场大泳池
        {
            UTF8String(u8"运动场大泳池"),
            UTF8String(u8"Sport Largeswimmingpool"),
            1.0000f, 0.8200f, 0.3162f, 0.7943f, 1.0000f, 5.4900f, 1.3100f, 1.1400f, 0.4467f, 0.0390f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Sport Gymnasium - 运动场体育馆
        {
            UTF8String(u8"运动场体育馆"),
            UTF8String(u8"Sport Gymnasium"),
            1.0000f, 0.8100f, 0.3162f, 0.4467f, 0.8913f, 3.1400f, 1.0600f, 1.3500f, 0.3981f, 0.0290f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Sport Fullstadium - 运动场满座体育场
        {
            UTF8String(u8"运动场满座体育场"),
            UTF8String(u8"Sport Fullstadium"),
            1.0000f, 1.0000f, 0.3162f, 0.0708f, 0.7943f, 5.2500f, 0.1700f, 0.8000f, 0.1000f, 0.1880f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Sport Stadiumtannoy - 运动场广播系统
        {
            UTF8String(u8"运动场广播系统"),
            UTF8String(u8"Sport Stadiumtannoy"),
            1.0000f, 0.7800f, 0.3162f, 0.5623f, 0.5012f, 2.5300f, 0.8800f, 0.6800f, 0.2818f, 0.2300f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Prefab Workshop - 预制车间
        {
            UTF8String(u8"预制车间"),
            UTF8String(u8"Prefab Workshop"),
            0.4287f, 1.0000f, 0.3162f, 0.1413f, 0.3981f, 0.7600f, 1.0000f, 1.0000f, 1.0000f, 0.0120f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Prefab Schoolroom - 预制教室
        {
            UTF8String(u8"预制教室"),
            UTF8String(u8"Prefab Schoolroom"),
            0.4022f, 0.6900f, 0.3162f, 0.6310f, 0.5012f, 0.9800f, 0.4500f, 0.1800f, 1.4125f, 0.0170f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Prefab Practiseroom - 预制练习室
        {
            UTF8String(u8"预制练习室"),
            UTF8String(u8"Prefab Practiseroom"),
            0.4022f, 0.8700f, 0.3162f, 0.3981f, 0.5012f, 1.1200f, 0.5600f, 0.1800f, 1.2589f, 0.0100f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Prefab Outhouse - 预制外屋
        {
            UTF8String(u8"预制外屋"),
            UTF8String(u8"Prefab Outhouse"),
            1.0000f, 0.8200f, 0.3162f, 0.1122f, 0.1585f, 1.3800f, 0.3800f, 0.3500f, 0.8913f, 0.0240f, { 0.0000f, 0.0000f, -0.0000f
        },
        // Prefab Caravan - 预制拖车
        {
            UTF8String(u8"预制拖车"),
            UTF8String(u8"Prefab Caravan"),
            1.0000f, 1.0000f, 0.3162f, 0.0891f, 0.1259f, 0.4300f, 1.5000f, 1.0000f, 1.0000f, 0.0120f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Dome Tomb - 圆顶墓穴
        {
            UTF8String(u8"圆顶墓穴"),
            UTF8String(u8"Dome Tomb"),
            1.0000f, 0.7900f, 0.3162f, 0.3548f, 0.2239f, 4.1800f, 0.2100f, 0.1000f, 0.3868f, 0.0300f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Pipe Small - 小管道
        {
            UTF8String(u8"小管道"),
            UTF8String(u8"Pipe Small"),
            1.0000f, 1.0000f, 0.3162f, 0.3548f, 0.2239f, 5.0400f, 0.1000f, 0.1000f, 0.5012f, 0.0320f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Dome Saintpauls - 圣保罗大教堂
        {
            UTF8String(u8"圣保罗大教堂"),
            UTF8String(u8"Dome Saintpauls"),
            1.0000f, 0.8700f, 0.3162f, 0.3548f, 0.2239f, 10.4800f, 0.1900f, 0.1000f, 0.1778f, 0.0900f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Pipe Longthin - 细长管道
        {
            UTF8String(u8"细长管道"),
            UTF8String(u8"Pipe Longthin"),
            0.2560f, 0.9100f, 0.3162f, 0.4467f, 0.2818f, 9.2100f, 0.1800f, 0.1000f, 0.7079f, 0.0100f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Pipe Large - 大管道
        {
            UTF8String(u8"大管道"),
            UTF8String(u8"Pipe Large"),
            1.0000f, 1.0000f, 0.3162f, 0.3548f, 0.2239f, 8.4500f, 0.1000f, 0.1000f, 0.3981f, 0.0460f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Pipe Resonant - 共鸣管道
        {
            UTF8String(u8"共鸣管道"),
            UTF8String(u8"Pipe Resonant"),
            0.1373f, 0.9100f, 0.3162f, 0.4467f, 0.2818f, 6.8100f, 0.1800f, 0.1000f, 0.7079f, 0.0100f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Outdoors Backyard - 户外后院
        {
            UTF8String(u8"户外后院"),
            UTF8String(u8"Outdoors Backyard"),
            1.0000f, 0.4500f, 0.3162f, 0.2512f, 0.5012f, 1.1200f, 0.3400f, 0.4600f, 0.4467f, 0.0690f, { 0.0000f, 0.0000f, -0.0000f
        },
        // Outdoors Rollingplains - 户外平原
        {
            UTF8String(u8"户外平原"),
            UTF8String(u8"Outdoors Rollingplains"),
            1.0000f, 0.0000f, 0.3162f, 0.0112f, 0.6310f, 2.1300f, 0.2100f, 0.4600f, 0.1778f, 0.3000f, { 0.0000f, 0.0000f, -0.0000f
        },
        // Outdoors Deepcanyon - 户外深峡谷
        {
            UTF8String(u8"户外深峡谷"),
            UTF8String(u8"Outdoors Deepcanyon"),
            1.0000f, 0.7400f, 0.3162f, 0.1778f, 0.6310f, 3.8900f, 0.2100f, 0.4600f, 0.3162f, 0.2230f, { 0.0000f, 0.0000f, -0.0000f
        },
        // Outdoors Creek - 户外小溪
        {
            UTF8String(u8"户外小溪"),
            UTF8String(u8"Outdoors Creek"),
            1.0000f, 0.3500f, 0.3162f, 0.1778f, 0.5012f, 2.1300f, 0.2100f, 0.4600f, 0.3981f, 0.1150f, { 0.0000f, 0.0000f, -0.0000f
        },
        // Outdoors Valley - 户外山谷
        {
            UTF8String(u8"户外山谷"),
            UTF8String(u8"Outdoors Valley"),
            1.0000f, 0.2800f, 0.3162f, 0.0282f, 0.1585f, 2.8800f, 0.2600f, 0.3500f, 0.1413f, 0.2630f, { 0.0000f, 0.0000f, -0.0000f
        },
        // Mood Heaven - 氛围天堂
        {
            UTF8String(u8"氛围天堂"),
            UTF8String(u8"Mood Heaven"),
            1.0000f, 0.9400f, 0.3162f, 0.7943f, 0.4467f, 5.0400f, 1.1200f, 0.5600f, 0.2427f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Mood Hell - 氛围地狱
        {
            UTF8String(u8"氛围地狱"),
            UTF8String(u8"Mood Hell"),
            1.0000f, 0.5700f, 0.3162f, 0.3548f, 0.4467f, 3.5700f, 0.4900f, 2.0000f, 0.0000f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Mood Memory - 氛围记忆
        {
            UTF8String(u8"氛围记忆"),
            UTF8String(u8"Mood Memory"),
            1.0000f, 0.8500f, 0.3162f, 0.6310f, 0.3548f, 4.0600f, 0.8200f, 0.5600f, 0.0398f, 0.0000f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Driving Commentator - 驾驶解说员
        {
            UTF8String(u8"驾驶解说员"),
            UTF8String(u8"Driving Commentator"),
            1.0000f, 0.0000f, 0.3162f, 0.5623f, 0.5012f, 2.4200f, 0.8800f, 0.6800f, 0.1995f, 0.0930f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Driving Pitgarage - 驾驶维修站
        {
            UTF8String(u8"驾驶维修站"),
            UTF8String(u8"Driving Pitgarage"),
            0.4287f, 0.5900f, 0.3162f, 0.7079f, 0.5623f, 1.7200f, 0.9300f, 0.8700f, 0.5623f, 0.0000f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Driving Incar Racer - 驾驶赛车内
        {
            UTF8String(u8"驾驶赛车内"),
            UTF8String(u8"Driving Incar Racer"),
            0.0832f, 0.8000f, 0.3162f, 1.0000f, 0.7943f, 0.1700f, 2.0000f, 0.4100f, 1.7783f, 0.0070f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Driving Incar Sports - 驾驶跑车内
        {
            UTF8String(u8"驾驶跑车内"),
            UTF8String(u8"Driving Incar Sports"),
            0.0832f, 0.8000f, 0.3162f, 0.6310f, 1.0000f, 0.1700f, 0.7500f, 0.4100f, 1.0000f, 0.0100f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Driving Incar Luxury - 驾驶豪华车内
        {
            UTF8String(u8"驾驶豪华车内"),
            UTF8String(u8"Driving Incar Luxury"),
            0.2560f, 1.0000f, 0.3162f, 0.1000f, 0.5012f, 0.1300f, 0.4100f, 0.4600f, 0.7943f, 0.0100f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Driving Fullgrandstand - 驾驶满座看台
        {
            UTF8String(u8"驾驶满座看台"),
            UTF8String(u8"Driving Fullgrandstand"),
            1.0000f, 1.0000f, 0.3162f, 0.2818f, 0.6310f, 3.0100f, 1.3700f, 1.2800f, 0.3548f, 0.0900f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Driving Emptygrandstand - 驾驶空看台
        {
            UTF8String(u8"驾驶空看台"),
            UTF8String(u8"Driving Emptygrandstand"),
            1.0000f, 1.0000f, 0.3162f, 1.0000f, 0.7943f, 4.6200f, 1.7500f, 1.4000f, 0.2082f, 0.0900f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Driving Tunnel - 驾驶隧道
        {
            UTF8String(u8"驾驶隧道"),
            UTF8String(u8"Driving Tunnel"),
            1.0000f, 0.8100f, 0.3162f, 0.3981f, 0.8913f, 3.4200f, 0.9400f, 1.3100f, 0.7079f, 0.0510f, { 0.0000f, 0.0000f, 0.0000f
        },
        // City Streets - 城市街道
        {
            UTF8String(u8"城市街道"),
            UTF8String(u8"City Streets"),
            1.0000f, 0.7800f, 0.3162f, 0.7079f, 0.8913f, 1.7900f, 1.1200f, 0.9100f, 0.2818f, 0.0460f, { 0.0000f, 0.0000f, 0.0000f
        },
        // City Subway - 城市地铁
        {
            UTF8String(u8"城市地铁"),
            UTF8String(u8"City Subway"),
            1.0000f, 0.7400f, 0.3162f, 0.7079f, 0.8913f, 3.0100f, 1.2300f, 0.9100f, 0.7079f, 0.0460f, { 0.0000f, 0.0000f, 0.0000f
        },
        // City Museum - 城市博物馆
        {
            UTF8String(u8"城市博物馆"),
            UTF8String(u8"City Museum"),
            1.0000f, 0.8200f, 0.3162f, 0.1778f, 0.1778f, 3.2800f, 1.4000f, 0.5700f, 0.2512f, 0.0390f, { 0.0000f, 0.0000f, -0.0000f
        },
        // City Library - 城市图书馆
        {
            UTF8String(u8"城市图书馆"),
            UTF8String(u8"City Library"),
            1.0000f, 0.8200f, 0.3162f, 0.2818f, 0.0891f, 2.7600f, 0.8900f, 0.4100f, 0.3548f, 0.0290f, { 0.0000f, 0.0000f, -0.0000f
        },
        // City Underpass - 城市地下通道
        {
            UTF8String(u8"城市地下通道"),
            UTF8String(u8"City Underpass"),
            1.0000f, 0.8200f, 0.3162f, 0.4467f, 0.8913f, 3.5700f, 1.1200f, 0.9100f, 0.3981f, 0.0590f, { 0.0000f, 0.0000f, 0.0000f
        },
        // City Abandoned - 城市废弃区
        {
            UTF8String(u8"城市废弃区"),
            UTF8String(u8"City Abandoned"),
            1.0000f, 0.6900f, 0.3162f, 0.7943f, 0.8913f, 3.2800f, 1.1700f, 0.9100f, 0.4467f, 0.0440f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Dustyroom - 尘土飞扬的房间
        {
            UTF8String(u8"尘土飞扬的房间"),
            UTF8String(u8"Dustyroom"),
            0.3645f, 0.5600f, 0.3162f, 0.7943f, 0.7079f, 1.7900f, 0.3800f, 0.2100f, 0.5012f, 0.0020f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Chapel - 小教堂
        {
            UTF8String(u8"小教堂"),
            UTF8String(u8"Chapel"),
            1.0000f, 0.8400f, 0.3162f, 0.5623f, 1.0000f, 4.6200f, 0.6400f, 1.2300f, 0.4467f, 0.0320f, { 0.0000f, 0.0000f, 0.0000f
        },
        // Smallwaterroom - 小水房
        {
            UTF8String(u8"小水房"),
            UTF8String(u8"Smallwaterroom"),
            1.0000f, 0.7000f, 0.3162f, 0.4477f, 1.0000f, 1.5100f, 1.2500f, 1.1400f, 0.8913f, 0.0200f, { 0.0000f, 0.0000f, 0.0000f
        }
    };

    const ReverbPresetProperties *GetReverbPresetProperties(ReverbPreset preset)
    {
        int index = static_cast<int>(preset);
        if (index < 0 || index >= GetReverbPresetCount())
            return nullptr;
        return &reverb_presets[index];
    }

    int GetReverbPresetCount()
    {
        return sizeof(reverb_presets) / sizeof(reverb_presets[0]);
    }
}//namespace hgl
