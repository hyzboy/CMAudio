#ifndef HGL_MIDI_INSTRUMENT_INCLUDE
#define HGL_MIDI_INSTRUMENT_INCLUDE

namespace hgl
{
    /**
     * MIDI General MIDI Level 1 乐器编号定义
     * MIDI General MIDI Level 1 Instrument Program Numbers
     * 
     * 使用 enum class 提供类型安全和命名空间隔离
     * Using enum class for type safety and namespace isolation
     */
    enum class MIDIInstrument : int
    {
        // Piano (钢琴) - Programs 0-7
        AcousticGrandPiano = 0,     // 大钢琴
        BrightAcousticPiano = 1,    // 明亮的钢琴
        ElectricGrandPiano = 2,     // 电钢琴
        HonkyTonkPiano = 3,         // 酒吧钢琴
        ElectricPiano1 = 4,         // 电钢琴1
        ElectricPiano2 = 5,         // 电钢琴2
        Harpsichord = 6,            // 羽管键琴
        Clavinet = 7,               // 克拉维琴

        // Chromatic Percussion (半音阶打击乐器) - Programs 8-15
        Celesta = 8,                // 钢片琴
        Glockenspiel = 9,           // 钟琴
        MusicBox = 10,              // 八音盒
        Vibraphone = 11,            // 颤音琴
        Marimba = 12,               // 马林巴琴
        Xylophone = 13,             // 木琴
        TubularBells = 14,          // 管钟
        Dulcimer = 15,              // 扬琴

        // Organ (风琴) - Programs 16-23
        DrawbarOrgan = 16,          // 击杆风琴
        PercussiveOrgan = 17,       // 打击式风琴
        RockOrgan = 18,             // 摇滚风琴
        ChurchOrgan = 19,           // 教堂风琴
        ReedOrgan = 20,             // 簧风琴
        Accordion = 21,             // 手风琴
        Harmonica = 22,             // 口琴
        TangoAccordion = 23,        // 探戈手风琴

        // Guitar (吉他) - Programs 24-31
        AcousticGuitarNylon = 24,   // 尼龙弦吉他
        AcousticGuitarSteel = 25,   // 钢弦吉他
        ElectricGuitarJazz = 26,    // 爵士电吉他
        ElectricGuitarClean = 27,   // 清音电吉他
        ElectricGuitarMuted = 28,   // 闷音电吉他
        OverdrivenGuitar = 29,      // 失真吉他
        DistortionGuitar = 30,      // 重失真吉他
        GuitarHarmonics = 31,       // 吉他和声

        // Bass (贝斯) - Programs 32-39
        AcousticBass = 32,          // 原声贝斯
        ElectricBassFinger = 33,    // 指弹电贝斯
        ElectricBassPick = 34,      // 拨片电贝斯
        FretlessBass = 35,          // 无品贝斯
        SlapBass1 = 36,             // 击弦贝斯1
        SlapBass2 = 37,             // 击弦贝斯2
        SynthBass1 = 38,            // 合成贝斯1
        SynthBass2 = 39,            // 合成贝斯2

        // Strings (弦乐) - Programs 40-47
        Violin = 40,                // 小提琴
        Viola = 41,                 // 中提琴
        Cello = 42,                 // 大提琴
        Contrabass = 43,            // 低音提琴
        TremoloStrings = 44,        // 弦乐颤音
        PizzicatoStrings = 45,      // 弦乐拨奏
        OrchestralHarp = 46,        // 竖琴
        Timpani = 47,               // 定音鼓

        // Ensemble (合奏) - Programs 48-55
        StringEnsemble1 = 48,       // 弦乐合奏1
        StringEnsemble2 = 49,       // 弦乐合奏2
        SynthStrings1 = 50,         // 合成弦乐1
        SynthStrings2 = 51,         // 合成弦乐2
        ChoirAahs = 52,             // 人声合唱
        VoiceOohs = 53,             // 人声"呜"
        SynthVoice = 54,            // 合成人声
        OrchestraHit = 55,          // 管弦乐敲击

        // Brass (铜管) - Programs 56-63
        Trumpet = 56,               // 小号
        Trombone = 57,              // 长号
        Tuba = 58,                  // 大号
        MutedTrumpet = 59,          // 弱音小号
        FrenchHorn = 60,            // 圆号
        BrassSection = 61,          // 铜管组
        SynthBrass1 = 62,           // 合成铜管1
        SynthBrass2 = 63,           // 合成铜管2

        // Reed (簧管) - Programs 64-71
        SopranoSax = 64,            // 高音萨克斯
        AltoSax = 65,               // 中音萨克斯
        TenorSax = 66,              // 次中音萨克斯
        BaritoneSax = 67,           // 上低音萨克斯
        Oboe = 68,                  // 双簧管
        EnglishHorn = 69,           // 英国管
        Bassoon = 70,               // 巴松管
        Clarinet = 71,              // 单簧管

        // Pipe (管乐) - Programs 72-79
        Piccolo = 72,               // 短笛
        Flute = 73,                 // 长笛
        Recorder = 74,              // 竖笛
        PanFlute = 75,              // 排箫
        BlownBottle = 76,           // 瓶笛
        Shakuhachi = 77,            // 尺八
        Whistle = 78,               // 口哨
        Ocarina = 79,               // 陶笛

        // Synth Lead (合成主音) - Programs 80-87
        Lead1Square = 80,           // 方波主音
        Lead2Sawtooth = 81,         // 锯齿波主音
        Lead3Calliope = 82,         // 汽笛风琴主音
        Lead4Chiff = 83,            // 吹管主音
        Lead5Charang = 84,          // 唱诗班主音
        Lead6Voice = 85,            // 人声主音
        Lead7Fifths = 86,           // 五度主音
        Lead8BassLead = 87,         // 贝斯主音

        // Synth Pad (合成音色) - Programs 88-95
        Pad1NewAge = 88,            // 新世纪音色
        Pad2Warm = 89,              // 温暖音色
        Pad3Polysynth = 90,         // 复音合成器
        Pad4Choir = 91,             // 合唱音色
        Pad5Bowed = 92,             // 弓弦音色
        Pad6Metallic = 93,          // 金属音色
        Pad7Halo = 94,              // 光环音色
        Pad8Sweep = 95,             // 扫描音色

        // Synth Effects (合成效果) - Programs 96-103
        FX1Rain = 96,               // 雨声
        FX2Soundtrack = 97,         // 电影音轨
        FX3Crystal = 98,            // 水晶
        FX4Atmosphere = 99,         // 大气
        FX5Brightness = 100,        // 明亮
        FX6Goblins = 101,           // 小妖精
        FX7Echoes = 102,            // 回声
        FX8SciFi = 103,             // 科幻

        // Ethnic (民族乐器) - Programs 104-111
        Sitar = 104,                // 西塔琴
        Banjo = 105,                // 班卓琴
        Shamisen = 106,             // 三味线
        Koto = 107,                 // 古筝
        Kalimba = 108,              // 卡林巴琴
        Bagpipe = 109,              // 风笛
        Fiddle = 110,               // 小提琴（民乐）
        Shanai = 111,               // 唢呐

        // Percussive (打击乐) - Programs 112-119
        TinkleBell = 112,           // 叮当铃
        Agogo = 113,                // 阿哥哥鼓
        SteelDrums = 114,           // 钢鼓
        Woodblock = 115,            // 木鱼
        TaikoDrum = 116,            // 太鼓
        MelodicTom = 117,           // 旋律鼓
        SynthDrum = 118,            // 合成鼓
        ReverseCymbal = 119,        // 反向钹

        // Sound Effects (声音效果) - Programs 120-127
        GuitarFretNoise = 120,      // 吉他滑弦噪音
        BreathNoise = 121,          // 呼吸声
        Seashore = 122,             // 海浪声
        BirdTweet = 123,            // 鸟鸣
        TelephoneRing = 124,        // 电话铃声
        Helicopter = 125,           // 直升机
        Applause = 126,             // 掌声
        Gunshot = 127               // 枪声
    };

    /**
     * 将 MIDIInstrument 枚举转换为整数
     * Convert MIDIInstrument enum to integer
     */
    inline int ToMIDIProgram(MIDIInstrument instrument)
    {
        return static_cast<int>(instrument);
    }

    /**
     * 将整数转换为 MIDIInstrument 枚举
     * Convert integer to MIDIInstrument enum
     */
    inline MIDIInstrument ToMIDIInstrument(int program)
    {
        if (program < 0 || program > 127)
            program = 0;
        return static_cast<MIDIInstrument>(program);
    }

    /**
     * 获取乐器的英文名称
     * Get the English name of the instrument
     */
    const char* GetMIDIInstrumentName(MIDIInstrument instrument);

    /**
     * 获取乐器的中文名称
     * Get the Chinese name of the instrument
     */
    const char* GetMIDIInstrumentNameCN(MIDIInstrument instrument);
}

#endif // HGL_MIDI_INSTRUMENT_INCLUDE
