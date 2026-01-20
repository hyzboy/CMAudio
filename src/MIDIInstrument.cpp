#include <hgl/audio/MIDIInstrument.h>

namespace hgl
{
    static const char* MIDI_INSTRUMENT_NAMES_EN[128] = {
        // Piano (0-7)
        "Acoustic Grand Piano", "Bright Acoustic Piano", "Electric Grand Piano", "Honky-tonk Piano",
        "Electric Piano 1", "Electric Piano 2", "Harpsichord", "Clavinet",
        
        // Chromatic Percussion (8-15)
        "Celesta", "Glockenspiel", "Music Box", "Vibraphone",
        "Marimba", "Xylophone", "Tubular Bells", "Dulcimer",
        
        // Organ (16-23)
        "Drawbar Organ", "Percussive Organ", "Rock Organ", "Church Organ",
        "Reed Organ", "Accordion", "Harmonica", "Tango Accordion",
        
        // Guitar (24-31)
        "Acoustic Guitar (nylon)", "Acoustic Guitar (steel)", "Electric Guitar (jazz)", "Electric Guitar (clean)",
        "Electric Guitar (muted)", "Overdriven Guitar", "Distortion Guitar", "Guitar Harmonics",
        
        // Bass (32-39)
        "Acoustic Bass", "Electric Bass (finger)", "Electric Bass (pick)", "Fretless Bass",
        "Slap Bass 1", "Slap Bass 2", "Synth Bass 1", "Synth Bass 2",
        
        // Strings (40-47)
        "Violin", "Viola", "Cello", "Contrabass",
        "Tremolo Strings", "Pizzicato Strings", "Orchestral Harp", "Timpani",
        
        // Ensemble (48-55)
        "String Ensemble 1", "String Ensemble 2", "Synth Strings 1", "Synth Strings 2",
        "Choir Aahs", "Voice Oohs", "Synth Voice", "Orchestra Hit",
        
        // Brass (56-63)
        "Trumpet", "Trombone", "Tuba", "Muted Trumpet",
        "French Horn", "Brass Section", "Synth Brass 1", "Synth Brass 2",
        
        // Reed (64-71)
        "Soprano Sax", "Alto Sax", "Tenor Sax", "Baritone Sax",
        "Oboe", "English Horn", "Bassoon", "Clarinet",
        
        // Pipe (72-79)
        "Piccolo", "Flute", "Recorder", "Pan Flute",
        "Blown Bottle", "Shakuhachi", "Whistle", "Ocarina",
        
        // Synth Lead (80-87)
        "Lead 1 (square)", "Lead 2 (sawtooth)", "Lead 3 (calliope)", "Lead 4 (chiff)",
        "Lead 5 (charang)", "Lead 6 (voice)", "Lead 7 (fifths)", "Lead 8 (bass + lead)",
        
        // Synth Pad (88-95)
        "Pad 1 (new age)", "Pad 2 (warm)", "Pad 3 (polysynth)", "Pad 4 (choir)",
        "Pad 5 (bowed)", "Pad 6 (metallic)", "Pad 7 (halo)", "Pad 8 (sweep)",
        
        // Synth Effects (96-103)
        "FX 1 (rain)", "FX 2 (soundtrack)", "FX 3 (crystal)", "FX 4 (atmosphere)",
        "FX 5 (brightness)", "FX 6 (goblins)", "FX 7 (echoes)", "FX 8 (sci-fi)",
        
        // Ethnic (104-111)
        "Sitar", "Banjo", "Shamisen", "Koto",
        "Kalimba", "Bagpipe", "Fiddle", "Shanai",
        
        // Percussive (112-119)
        "Tinkle Bell", "Agogo", "Steel Drums", "Woodblock",
        "Taiko Drum", "Melodic Tom", "Synth Drum", "Reverse Cymbal",
        
        // Sound Effects (120-127)
        "Guitar Fret Noise", "Breath Noise", "Seashore", "Bird Tweet",
        "Telephone Ring", "Helicopter", "Applause", "Gunshot"
    };

    static const char* MIDI_INSTRUMENT_NAMES_CN[128] = {
        // Piano (0-7)
        "大钢琴", "明亮的钢琴", "电钢琴", "酒吧钢琴",
        "电钢琴1", "电钢琴2", "羽管键琴", "克拉维琴",
        
        // Chromatic Percussion (8-15)
        "钢片琴", "钟琴", "八音盒", "颤音琴",
        "马林巴琴", "木琴", "管钟", "扬琴",
        
        // Organ (16-23)
        "击杆风琴", "打击式风琴", "摇滚风琴", "教堂风琴",
        "簧风琴", "手风琴", "口琴", "探戈手风琴",
        
        // Guitar (24-31)
        "尼龙弦吉他", "钢弦吉他", "爵士电吉他", "清音电吉他",
        "闷音电吉他", "失真吉他", "重失真吉他", "吉他和声",
        
        // Bass (32-39)
        "原声贝斯", "指弹电贝斯", "拨片电贝斯", "无品贝斯",
        "击弦贝斯1", "击弦贝斯2", "合成贝斯1", "合成贝斯2",
        
        // Strings (40-47)
        "小提琴", "中提琴", "大提琴", "低音提琴",
        "弦乐颤音", "弦乐拨奏", "竖琴", "定音鼓",
        
        // Ensemble (48-55)
        "弦乐合奏1", "弦乐合奏2", "合成弦乐1", "合成弦乐2",
        "人声合唱", "人声'呜'", "合成人声", "管弦乐敲击",
        
        // Brass (56-63)
        "小号", "长号", "大号", "弱音小号",
        "圆号", "铜管组", "合成铜管1", "合成铜管2",
        
        // Reed (64-71)
        "高音萨克斯", "中音萨克斯", "次中音萨克斯", "上低音萨克斯",
        "双簧管", "英国管", "巴松管", "单簧管",
        
        // Pipe (72-79)
        "短笛", "长笛", "竖笛", "排箫",
        "瓶笛", "尺八", "口哨", "陶笛",
        
        // Synth Lead (80-87)
        "方波主音", "锯齿波主音", "汽笛风琴主音", "吹管主音",
        "唱诗班主音", "人声主音", "五度主音", "贝斯主音",
        
        // Synth Pad (88-95)
        "新世纪音色", "温暖音色", "复音合成器", "合唱音色",
        "弓弦音色", "金属音色", "光环音色", "扫描音色",
        
        // Synth Effects (96-103)
        "雨声", "电影音轨", "水晶", "大气",
        "明亮", "小妖精", "回声", "科幻",
        
        // Ethnic (104-111)
        "西塔琴", "班卓琴", "三味线", "古筝",
        "卡林巴琴", "风笛", "小提琴（民乐）", "唢呐",
        
        // Percussive (112-119)
        "叮当铃", "阿哥哥鼓", "钢鼓", "木鱼",
        "太鼓", "旋律鼓", "合成鼓", "反向钹",
        
        // Sound Effects (120-127)
        "吉他滑弦噪音", "呼吸声", "海浪声", "鸟鸣",
        "电话铃声", "直升机", "掌声", "枪声"
    };

    const char* GetMIDIInstrumentName(MIDIInstrument instrument)
    {
        int program = static_cast<int>(instrument);
        if (program < 0 || program > 127)
            return "Unknown";
        return MIDI_INSTRUMENT_NAMES_EN[program];
    }

    const char* GetMIDIInstrumentNameCN(MIDIInstrument instrument)
    {
        int program = static_cast<int>(instrument);
        if (program < 0 || program > 127)
            return "未知乐器";
        return MIDI_INSTRUMENT_NAMES_CN[program];
    }
}
