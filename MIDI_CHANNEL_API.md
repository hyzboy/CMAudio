# MIDI Channel Control API Documentation

## 概述 / Overview

**中文:**
MIDI 通道控制 API 提供了对 MIDI 播放的精细控制能力，允许开发者：
- 获取各通道的实时信息（乐器、音量、声像等）
- 动态更改通道的乐器和参数
- 静音/独奏特定通道
- 按通道单独解码音频数据

**English:**
The MIDI Channel Control API provides fine-grained control over MIDI playback, allowing developers to:
- Query real-time information about each channel (instrument, volume, pan, etc.)
- Dynamically change channel instruments and parameters
- Mute/solo specific channels
- Decode audio data per-channel separately

---

## API 接口 / API Interface

### 1. 通道信息查询 / Channel Information Query

#### `GetChannelCount()`
**功能 / Function:** 获取 MIDI 通道总数（标准 MIDI 为 16 个通道）  
**Function:** Get total number of MIDI channels (standard MIDI has 16 channels)

```cpp
AudioMidiChannelInterface* channelAPI = GetAudioMidiChannelInterface(decoder);
int channelCount = channelAPI->GetChannelCount();  // Returns 16
```

#### `GetChannelInfo(int channel, MidiChannelInfo* info)`
**功能 / Function:** 获取指定通道的详细信息  
**Function:** Get detailed information about a specific channel

**参数 / Parameters:**
- `channel`: 通道编号 (0-15) / Channel number (0-15)
- `info`: 输出参数，接收通道信息 / Output parameter to receive channel info

**返回 / Returns:** `true` 成功，`false` 失败 / `true` on success, `false` on failure

**MidiChannelInfo 结构 / Structure:**
```cpp
struct MidiChannelInfo
{
    int channel;                    // 通道编号 / Channel number (0-15)
    int program;                    // 当前乐器 / Current program/instrument (0-127)
    int bank;                       // 音色库编号 / Bank number (0-16383)
    float volume;                   // 通道音量 / Channel volume (0.0-1.0)
    float pan;                      // 声像位置 / Pan position (-1.0=left, 0.0=center, 1.0=right)
    bool muted;                     // 是否静音 / Is channel muted
    bool solo;                      // 是否独奏 / Is channel soloed
    int note_count;                 // 活跃音符数 / Number of active notes
    const char* instrument_name;    // 乐器名称 / Instrument name (if available)
};
```

**示例 / Example:**
```cpp
AudioMidiChannelInterface* channelAPI = /* ... */;

MidiChannelInfo info;
if (channelAPI->GetChannelInfo(0, &info))
{
    printf("Channel %d: Program %d, Volume %.2f, Pan %.2f\n",
           info.channel, info.program, info.volume, info.pan);
    printf("Muted: %s, Solo: %s\n", 
           info.muted ? "Yes" : "No",
           info.solo ? "Yes" : "No");
}
```

---

### 2. 通道控制 / Channel Control

#### `SetChannelProgram(int channel, int program)`
**功能 / Function:** 更改通道的乐器  
**Function:** Change the instrument on a channel

**参数 / Parameters:**
- `channel`: 通道编号 (0-15) / Channel number (0-15)
- `program`: 乐器编号 (0-127, GM 标准) / Program number (0-127, GM standard)

**GM 乐器编号参考 / GM Instrument Reference:**
- 0: Acoustic Grand Piano
- 24: Acoustic Guitar (nylon)
- 32: Acoustic Bass
- 40: Violin
- 56: Trumpet
- 73: Flute
- [完整列表见 General MIDI 规范 / Full list in General MIDI specification]

**示例 / Example:**
```cpp
// 将通道 0 改为小提琴
// Change channel 0 to violin
channelAPI->SetChannelProgram(0, 40);

// 将通道 9 保持为鼓组（通道 9 通常是打击乐）
// Keep channel 9 as drum kit (channel 9 is typically percussion)
channelAPI->SetChannelProgram(9, 0);  // Drum kit
```

#### `SetChannelBank(int channel, int bank)`
**功能 / Function:** 更改通道的音色库  
**Function:** Change the bank for a channel

**参数 / Parameters:**
- `channel`: 通道编号 (0-15) / Channel number (0-15)
- `bank`: 音色库编号 (0-16383) / Bank number (0-16383)

**说明 / Note:**  
音色库选择用于访问标准 GM 之外的音色。不同的合成器支持不同的音色库。  
Bank selection is used to access sounds beyond standard GM. Different synthesizers support different banks.

#### `SetChannelVolume(int channel, float volume)`
**功能 / Function:** 设置通道音量  
**Function:** Set channel volume

**参数 / Parameters:**
- `channel`: 通道编号 (0-15) / Channel number (0-15)
- `volume`: 音量 (0.0-1.0) / Volume (0.0-1.0)

**示例 / Example:**
```cpp
// 将钢琴通道音量设为 80%
// Set piano channel volume to 80%
channelAPI->SetChannelVolume(0, 0.8f);

// 降低鼓组音量到 50%
// Reduce drum volume to 50%
channelAPI->SetChannelVolume(9, 0.5f);
```

#### `SetChannelPan(int channel, float pan)`
**功能 / Function:** 设置通道声像位置  
**Function:** Set channel pan position

**参数 / Parameters:**
- `channel`: 通道编号 (0-15) / Channel number (0-15)
- `pan`: 声像位置 (-1.0=左, 0.0=中, 1.0=右) / Pan position (-1.0=left, 0.0=center, 1.0=right)

**示例 / Example:**
```cpp
// 将小提琴放在左侧
// Place violin on the left
channelAPI->SetChannelPan(1, -0.7f);

// 将大提琴放在右侧
// Place cello on the right
channelAPI->SetChannelPan(2, 0.7f);

// 钢琴居中
// Piano in center
channelAPI->SetChannelPan(0, 0.0f);
```

#### `MuteChannel(int channel, bool mute)`
**功能 / Function:** 静音/取消静音通道  
**Function:** Mute/unmute a channel

**参数 / Parameters:**
- `channel`: 通道编号 (0-15) / Channel number (0-15)
- `mute`: `true` 静音, `false` 取消静音 / `true` to mute, `false` to unmute

**示例 / Example:**
```cpp
// 静音鼓组
// Mute drums
channelAPI->MuteChannel(9, true);

// 取消静音
// Unmute
channelAPI->MuteChannel(9, false);
```

#### `SoloChannel(int channel, bool solo)`
**功能 / Function:** 独奏/取消独奏通道  
**Function:** Solo/unsolo a channel

**说明 / Note:**  
当任何通道被独奏时，所有其他未独奏的通道将被自动静音。  
When any channel is soloed, all other non-soloed channels are automatically muted.

**参数 / Parameters:**
- `channel`: 通道编号 (0-15) / Channel number (0-15)
- `solo`: `true` 独奏, `false` 取消独奏 / `true` to solo, `false` to unsolo

**示例 / Example:**
```cpp
// 只听钢琴
// Listen to piano only
channelAPI->SoloChannel(0, true);

// 同时听钢琴和小提琴
// Listen to piano and violin together
channelAPI->SoloChannel(0, true);
channelAPI->SoloChannel(1, true);

// 取消所有独奏
// Cancel all solos
for (int i = 0; i < 16; i++)
    channelAPI->SoloChannel(i, false);
```

---

### 3. 多通道解码 / Multi-Channel Decoding

#### `OpenMultiChannel(...)`
**功能 / Function:** 打开 MIDI 文件用于多通道解码  
**Function:** Open MIDI file for multi-channel decoding

```cpp
void* stream = channelAPI->OpenMultiChannel(data, size, &format, &freq, &time);
```

#### `ReadChannel(void* stream, int channel, char* pcm_data, uint buf_size)`
**功能 / Function:** 读取单个通道的 PCM 数据  
**Function:** Read PCM data for a single channel

**参数 / Parameters:**
- `stream`: 音频流指针 / Audio stream pointer
- `channel`: 要读取的通道 (0-15) / Channel to read (0-15)
- `pcm_data`: 输出 PCM 数据缓冲区 / Output PCM data buffer
- `buf_size`: 缓冲区大小 / Buffer size

**返回 / Returns:** 实际读取的字节数 / Number of bytes actually read

**示例 / Example:**
```cpp
// 只解码钢琴通道
// Decode piano channel only
char piano_buffer[4096];
uint read = channelAPI->ReadChannel(stream, 0, piano_buffer, sizeof(piano_buffer));

// 保存为独立文件或进一步处理
// Save to separate file or process further
```

#### `ReadChannels(void* stream, int* channels, int channel_count, char** pcm_data, uint buf_size)`
**功能 / Function:** 同时读取多个通道的 PCM 数据  
**Function:** Read PCM data for multiple channels simultaneously

**参数 / Parameters:**
- `stream`: 音频流指针 / Audio stream pointer
- `channels`: 要读取的通道数组 / Array of channels to read
- `channel_count`: 通道数量 / Number of channels
- `pcm_data`: 输出 PCM 数据缓冲区数组 / Array of output PCM buffers
- `buf_size`: 每个缓冲区大小 / Size of each buffer

**示例 / Example:**
```cpp
// 分别解码钢琴、小提琴、大提琴
// Decode piano, violin, cello separately
int channels[] = {0, 1, 2};
char* buffers[3];
buffers[0] = new char[4096];  // Piano
buffers[1] = new char[4096];  // Violin
buffers[2] = new char[4096];  // Cello

uint read = channelAPI->ReadChannels(stream, channels, 3, buffers, 4096);

// 现在可以分别处理每个通道
// Now can process each channel separately
// 例如：混合、分析、导出等
// Example: mixing, analysis, export, etc.
```

#### `CloseMultiChannel(void* stream)`
**功能 / Function:** 关闭多通道解码流  
**Function:** Close multi-channel decode stream

```cpp
channelAPI->CloseMultiChannel(stream);
```

---

## 完整使用示例 / Complete Usage Examples

### 示例 1: 动态混音器 / Example 1: Dynamic Mixer

**中文场景：** 实现一个简单的 MIDI 混音器，允许用户调整各通道音量  
**English Scenario:** Implement a simple MIDI mixer allowing users to adjust channel volumes

```cpp
#include <hgl/audio/AudioDecode.h>
#include <hgl/audio/AudioPlayer.h>

class MidiMixer
{
private:
    AudioPlayer player;
    AudioMidiChannelInterface* channelAPI;
    float channelVolumes[16];

public:
    MidiMixer()
    {
        // 初始化所有通道音量为 100%
        // Initialize all channel volumes to 100%
        for (int i = 0; i < 16; i++)
            channelVolumes[i] = 1.0f;
    }

    bool Load(const char* midiFile)
    {
        if (!player.Load(midiFile))
            return false;

        // 获取通道控制接口
        // Get channel control interface
        AudioDecode* decoder = player.GetDecoder();
        if (!GetAudioMidiChannelInterface(OS_TEXT("MIDI"), &channelAPI))
            return false;

        return true;
    }

    void SetChannelVolume(int channel, float volume)
    {
        if (channel < 0 || channel >= 16)
            return;

        channelVolumes[channel] = volume;
        if (channelAPI)
            channelAPI->SetChannelVolume(channel, volume);
    }

    void MuteChannel(int channel, bool mute)
    {
        if (channelAPI)
            channelAPI->MuteChannel(channel, mute);
    }

    void SoloChannel(int channel)
    {
        if (!channelAPI)
            return;

        // 取消所有独奏
        // Cancel all solos
        for (int i = 0; i < 16; i++)
            channelAPI->SoloChannel(i, false);

        // 独奏指定通道
        // Solo specified channel
        channelAPI->SoloChannel(channel, true);
    }

    void PrintChannelInfo()
    {
        if (!channelAPI)
            return;

        printf("=== MIDI Channel Info ===\n");
        for (int i = 0; i < channelAPI->GetChannelCount(); i++)
        {
            MidiChannelInfo info;
            if (channelAPI->GetChannelInfo(i, &info))
            {
                printf("Channel %2d: Program=%3d, Vol=%.2f, Pan=%+.2f, %s%s\n",
                       info.channel,
                       info.program,
                       info.volume,
                       info.pan,
                       info.muted ? "[MUTE] " : "",
                       info.solo ? "[SOLO] " : "");
            }
        }
    }

    void Play() { player.Play(true); }
    void Stop() { player.Stop(); }
};

// 使用示例 / Usage example
int main()
{
    MidiMixer mixer;
    
    if (!mixer.Load("song.mid"))
    {
        printf("Failed to load MIDI file\n");
        return 1;
    }

    mixer.Play();
    
    // 降低鼓组音量
    // Reduce drum volume
    mixer.SetChannelVolume(9, 0.5f);
    
    // 静音贝斯
    // Mute bass
    mixer.MuteChannel(1, true);
    
    // 独奏钢琴
    // Solo piano
    mixer.SoloChannel(0);
    
    // 显示通道信息
    // Display channel info
    mixer.PrintChannelInfo();
    
    return 0;
}
```

### 示例 2: 卡拉OK 模式 / Example 2: Karaoke Mode

**中文场景：** 移除人声通道，实现卡拉OK效果  
**English Scenario:** Remove vocal channel for karaoke effect

```cpp
class KaraokePlayer
{
private:
    AudioPlayer player;
    AudioMidiChannelInterface* channelAPI;
    int vocalChannel;  // 人声通道 / Vocal channel

public:
    KaraokePlayer() : vocalChannel(-1) {}

    bool Load(const char* midiFile, int vocal_ch = 4)
    {
        vocalChannel = vocal_ch;
        
        if (!player.Load(midiFile))
            return false;

        AudioDecode* decoder = player.GetDecoder();
        if (!GetAudioMidiChannelInterface(OS_TEXT("MIDI"), &channelAPI))
            return false;

        return true;
    }

    void EnableKaraoke(bool enable)
    {
        if (channelAPI && vocalChannel >= 0)
        {
            channelAPI->MuteChannel(vocalChannel, enable);
        }
    }

    void Play()
    {
        player.Play(true);
        EnableKaraoke(true);  // 默认开启卡拉OK模式 / Enable karaoke by default
    }
};
```

### 示例 3: 乐器学习工具 / Example 3: Music Learning Tool

**中文场景：** 隔离特定乐器用于练习  
**English Scenario:** Isolate specific instruments for practice

```cpp
class InstrumentLearner
{
private:
    AudioPlayer player;
    AudioMidiChannelInterface* channelAPI;

public:
    bool Load(const char* midiFile)
    {
        if (!player.Load(midiFile))
            return false;

        AudioDecode* decoder = player.GetDecoder();
        return GetAudioMidiChannelInterface(OS_TEXT("MIDI"), &channelAPI);
    }

    void PracticeMode(int instrumentChannel, float backgroundVolume = 0.3f)
    {
        if (!channelAPI)
            return;

        // 降低所有通道音量作为背景
        // Reduce all channels as background
        for (int i = 0; i < 16; i++)
        {
            if (i == instrumentChannel)
                channelAPI->SetChannelVolume(i, 1.0f);  // 目标乐器全音量 / Target at full volume
            else
                channelAPI->SetChannelVolume(i, backgroundVolume);  // 其他为背景 / Others as background
        }
    }

    void ChangeInstrument(int channel, int newProgram)
    {
        if (channelAPI)
        {
            channelAPI->SetChannelProgram(channel, newProgram);
            printf("Changed channel %d to program %d\n", channel, newProgram);
        }
    }

    void Play() { player.Play(true); }
};

// 使用示例 / Usage
int main()
{
    InstrumentLearner learner;
    learner.Load("lesson.mid");
    
    // 练习钢琴部分，其他乐器作为轻背景
    // Practice piano part with other instruments as light background
    learner.PracticeMode(0, 0.2f);
    
    learner.Play();
    
    return 0;
}
```

### 示例 4: 多轨导出 / Example 4: Multi-Track Export

**中文场景：** 将 MIDI 各通道导出为独立 WAV 文件  
**English Scenario:** Export each MIDI channel as separate WAV file

```cpp
#include <fstream>

class MidiExporter
{
private:
    AudioMidiChannelInterface* channelAPI;

    void WriteWavHeader(std::ofstream& file, int dataSize, int sampleRate, int channels)
    {
        // WAV 文件头 / WAV header
        file.write("RIFF", 4);
        int fileSize = dataSize + 36;
        file.write((char*)&fileSize, 4);
        file.write("WAVE", 4);
        file.write("fmt ", 4);
        int fmtSize = 16;
        file.write((char*)&fmtSize, 4);
        short format = 1;  // PCM
        file.write((char*)&format, 2);
        short numChannels = channels;
        file.write((char*)&numChannels, 2);
        file.write((char*)&sampleRate, 4);
        int byteRate = sampleRate * channels * 2;
        file.write((char*)&byteRate, 4);
        short blockAlign = channels * 2;
        file.write((char*)&blockAlign, 2);
        short bitsPerSample = 16;
        file.write((char*)&bitsPerSample, 2);
        file.write("data", 4);
        file.write((char*)&dataSize, 4);
    }

public:
    bool ExportChannel(const char* midiFile, int channel, const char* outputWav)
    {
        // 加载 MIDI 文件
        // Load MIDI file
        FILE* f = fopen(midiFile, "rb");
        if (!f) return false;
        
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        char* data = new char[size];
        fread(data, 1, size, f);
        fclose(f);

        // 打开多通道解码
        // Open multi-channel decode
        if (!GetAudioMidiChannelInterface(OS_TEXT("MIDI"), &channelAPI))
        {
            delete[] data;
            return false;
        }

        ALenum format;
        ALsizei freq;
        double time;
        void* stream = channelAPI->OpenMultiChannel(
            (ALbyte*)data, size, &format, &freq, &time);

        if (!stream)
        {
            delete[] data;
            return false;
        }

        // 打开输出文件
        // Open output file
        std::ofstream outFile(outputWav, std::ios::binary);
        if (!outFile)
        {
            channelAPI->CloseMultiChannel(stream);
            delete[] data;
            return false;
        }

        // 预留WAV头空间
        // Reserve space for WAV header
        char header[44] = {0};
        outFile.write(header, 44);

        // 读取并写入通道数据
        // Read and write channel data
        char buffer[4096];
        int totalSize = 0;
        uint read;
        
        while ((read = channelAPI->ReadChannel(stream, channel, buffer, sizeof(buffer))) > 0)
        {
            outFile.write(buffer, read);
            totalSize += read;
        }

        // 回写正确的WAV头
        // Write correct WAV header
        outFile.seekp(0);
        WriteWavHeader(outFile, totalSize, freq, 2);  // Stereo

        outFile.close();
        channelAPI->CloseMultiChannel(stream);
        delete[] data;

        printf("Exported channel %d to %s (%d bytes)\n", channel, outputWav, totalSize);
        return true;
    }

    bool ExportAllChannels(const char* midiFile, const char* outputDir)
    {
        for (int i = 0; i < 16; i++)
        {
            char filename[256];
            sprintf(filename, "%s/channel_%02d.wav", outputDir, i);
            
            if (!ExportChannel(midiFile, i, filename))
            {
                printf("Failed to export channel %d\n", i);
            }
        }
        return true;
    }
};
```

---

## 注意事项 / Important Notes

### 性能考虑 / Performance Considerations

**中文:**
1. **多通道解码开销**: 按通道单独解码需要多次渲染，性能开销较大。建议仅用于离线处理或导出。
2. **实时控制**: 动态更改通道参数（音量、声像等）的性能开销很小，可用于实时应用。
3. **Solo模式**: 独奏模式会自动静音其他通道，多次切换可能有短暂延迟。

**English:**
1. **Multi-channel decode overhead**: Decoding channels separately requires multiple renders, which is expensive. Recommended for offline processing or export only.
2. **Real-time control**: Dynamically changing channel parameters (volume, pan, etc.) has minimal overhead and can be used in real-time applications.
3. **Solo mode**: Solo mode automatically mutes other channels; multiple switches may have brief delays.

### 插件兼容性 / Plugin Compatibility

**中文:**
- **FluidSynth**: 完整支持所有通道功能
- **TinySoundFont**: 支持基本通道控制，部分高级功能有限
- **WildMIDI/Timidity**: 支持音量和静音，其他功能有限
- **OPNMIDI/ADLMIDI**: 芯片模拟器，通道控制能力取决于芯片特性

**English:**
- **FluidSynth**: Full support for all channel features
- **TinySoundFont**: Supports basic channel control, some advanced features limited
- **WildMIDI/Timidity**: Supports volume and mute, other features limited
- **OPNMIDI/ADLMIDI**: Chip emulators, channel control depends on chip characteristics

### 通道分配建议 / Channel Assignment Guidelines

**标准 MIDI 通道分配 / Standard MIDI Channel Assignment:**
- **Channel 0-8**: 旋律乐器 / Melodic instruments
- **Channel 9**: 打击乐（鼓组）/ Percussion (drum kit)
- **Channel 10-15**: 旋律乐器 / Melodic instruments

---

## API 版本 / API Version

- **接口版本 / Interface Version**: 5
- **最低 CMAudio 版本 / Minimum CMAudio Version**: 1.0
- **支持插件 / Supported Plugins**: FluidSynth (full), TinySoundFont (partial), Others (basic)

---

## 相关文档 / Related Documentation

- **MIDI_CONFIG_API.md** - MIDI 基础配置 API / Basic MIDI configuration API
- **QUICK_REFERENCE.md** - 快速参考指南 / Quick reference guide
- **AudioPlayer_Analysis.md** - 音频播放器架构分析 / AudioPlayer architecture analysis

---

## 问题排查 / Troubleshooting

### Q: GetChannelInfo 返回空数据 / GetChannelInfo returns empty data
**A:** 确保在 MIDI 文件加载并开始播放后调用。某些插件需要先处理 MIDI 事件才能获取通道信息。  
**A:** Ensure calling after MIDI file is loaded and playback started. Some plugins need to process MIDI events first.

### Q: 多通道解码没有声音 / Multi-channel decode produces no audio
**A:** 检查通道是否被静音。使用 `GetChannelInfo` 确认通道状态。  
**A:** Check if channels are muted. Use `GetChannelInfo` to confirm channel status.

### Q: SetChannelProgram 不生效 / SetChannelProgram has no effect
**A:** 某些 MIDI 文件在播放过程中会重新发送 Program Change 消息。考虑在 MIDI 文件预处理时修改。  
**A:** Some MIDI files resend Program Change messages during playback. Consider modifying during MIDI file preprocessing.

---

*文档最后更新 / Last Updated: 2026-01-20*
