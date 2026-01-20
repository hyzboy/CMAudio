# MIDIPlayer 专业MIDI播放器 / Professional MIDI Player

[English Version](#english-version) | [中文版本](#chinese-version)

---

## Chinese Version

### 简介

`MIDIPlayer` 是专为MIDI文件设计的专业播放器类，提供完整的MIDI控制功能。与通用的 `AudioPlayer` 不同，`MIDIPlayer` 专注于MIDI特有的功能，如通道控制、乐器更换、音色库选择等。

### 核心特性

- ✅ **实时通道控制** - 动态调整每个通道的音量、声像、静音、独奏
- ✅ **动态乐器更换** - 运行时更改通道的乐器（Program Change）
- ✅ **音色库管理** - 支持SoundFont (.sf2) 和 FM芯片音色库 (.wopn/.wopl)
- ✅ **通道信息查询** - 获取每个通道的详细状态信息
- ✅ **多通道分离解码** - 将各通道单独导出为PCM数据
- ✅ **独立播放线程** - 不阻塞主线程
- ✅ **空间音频支持** - 3D定位、速度、方向等
- ✅ **淡入淡出** - 自动音量渐变
- ✅ **循环播放** - 无缝循环

### 与 AudioPlayer 的区别

| 特性 | AudioPlayer | MIDIPlayer |
|------|-------------|------------|
| **支持格式** | OGG, Opus, MIDI等多种格式 | 仅MIDI |
| **通道控制** | ❌ 不支持 | ✅ 完整支持 |
| **乐器更换** | ❌ 不支持 | ✅ 支持 |
| **音色库配置** | ❌ 不支持 | ✅ 支持 |
| **通道分离导出** | ❌ 不支持 | ✅ 支持 |
| **专业MIDI功能** | ❌ 基础播放 | ✅ 完整控制 |
| **适用场景** | 通用音频播放 | 专业MIDI应用 |

### 快速开始

#### 1. 基本播放

```cpp
#include <hgl/audio/MIDIPlayer.h>

// 创建播放器
MIDIPlayer player;

// 加载MIDI文件
if (player.Load("music.mid"))
{
    // 开始播放（循环）
    player.Play(true);
}
```

#### 2. 配置音色库

```cpp
// 使用 FluidSynth 或 TinySoundFont 时
player.SetSoundFont("/usr/share/sounds/sf2/FluidR3_GM.sf2");

// 使用 OPNMIDI 时（Sega Genesis 音色）
player.SetBank(74);  // Sonic 3 音色库

// 使用 ADLMIDI 时（DOS游戏音色）
player.SetBank(0);   // DMX OP2 (Doom 音色)
```

#### 3. 通道控制

```cpp
// 获取通道数量
int count = player.GetChannelCount();  // 通常为 16

// 查询通道信息
MidiChannelInfo info;
if (player.GetChannelInfo(0, &info))
{
    printf("Channel 0: Program %d, Volume %d, Pan %d\n",
           info.program, info.volume, info.pan);
}

// 更换通道乐器
player.SetChannelProgram(0, 40);  // 将通道0改为小提琴(40)

// 调整通道音量
player.SetChannelVolume(0, 100);  // 音量 0-127

// 设置声像
player.SetChannelPan(0, 0);      // 0=左, 64=中, 127=右

// 静音鼓组
player.SetChannelMute(9, true);

// 独奏钢琴通道
player.SetChannelSolo(0, true);
```

### 应用场景示例

#### 场景1：动态混音器

```cpp
#include <hgl/audio/MIDIPlayer.h>

class MIDIMixer
{
    MIDIPlayer player;
    
public:
    void Load(const os_char *filename)
    {
        player.Load(filename);
        player.Play(true);
    }
    
    // 调整特定乐器音量
    void SetInstrumentVolume(int channel, float volume)
    {
        player.SetChannelVolume(channel, (int)(volume * 127));
    }
    
    // 静音/取消静音
    void MuteInstrument(int channel, bool mute)
    {
        player.SetChannelMute(channel, mute);
    }
    
    // 独奏特定通道
    void SoloInstrument(int channel)
    {
        player.SetChannelSolo(channel, true);
    }
    
    // 调整立体声声像
    void SetPan(int channel, float pan)  // -1.0 到 1.0
    {
        int midi_pan = (int)((pan + 1.0f) * 63.5f);
        player.SetChannelPan(channel, midi_pan);
    }
};
```

#### 场景2：卡拉OK系统

```cpp
class KaraokePlayer
{
    MIDIPlayer player;
    
public:
    void LoadSong(const os_char *midi_file)
    {
        player.Load(midi_file);
        
        // 静音人声通道（通常在特定通道）
        MuteVocals();
        
        player.Play(false);  // 不循环
    }
    
    void MuteVocals()
    {
        // 静音通道 4 (通常用于主唱)
        player.SetChannelMute(4, true);
    }
    
    void UnmuteVocals()
    {
        player.SetChannelMute(4, false);
    }
    
    void AdjustKeyTranspose(int semitones)
    {
        // 通过调整音高实现变调
        float pitch = pow(2.0f, semitones / 12.0f);
        player.SetPitch(pitch);
    }
};
```

#### 场景3：音乐学习工具

```cpp
class MusicLearningTool
{
    MIDIPlayer player;
    
public:
    // 隔离特定乐器用于练习
    void IsolateInstrument(int channel)
    {
        // 独奏目标通道
        player.SetChannelSolo(channel, true);
        
        // 降低该通道音量（听起来更清晰）
        player.SetChannelVolume(channel, 80);
    }
    
    // 显示通道活动状态
    void ShowActiveChannels()
    {
        for (int i = 0; i < player.GetChannelCount(); i++)
        {
            if (player.GetChannelActivity(i))
            {
                MidiChannelInfo info;
                player.GetChannelInfo(i, &info);
                
                printf("Channel %d 活跃: %s\n", i, info.name);
            }
        }
    }
    
    // 慢速播放用于学习
    void SetPracticeSpeed(float speed)  // 0.5 = 一半速度
    {
        player.SetPitch(speed);
    }
};
```

#### 场景4：游戏动态音乐

```cpp
class GameMusicController
{
    MIDIPlayer player;
    
public:
    void LoadBattleMusic(const os_char *midi_file)
    {
        player.Load(midi_file);
        
        // 初始状态：只有环境音
        for (int i = 0; i < player.GetChannelCount(); i++)
        {
            player.SetChannelMute(i, true);
        }
        
        // 只开启环境音通道
        player.SetChannelMute(0, false);  // 和弦
        player.SetChannelMute(1, false);  // 低音
        
        player.Play(true);
    }
    
    void EnterCombat()
    {
        // 战斗开始：逐渐加入鼓和旋律
        player.SetChannelMute(9, false);  // 鼓组
        player.SetChannelMute(2, false);  // 旋律
        player.SetChannelVolume(9, 127);
        player.SetChannelVolume(2, 120);
    }
    
    void BossAppears()
    {
        // Boss出现：全力以赴
        for (int i = 0; i < player.GetChannelCount(); i++)
        {
            player.SetChannelMute(i, false);
            player.SetChannelVolume(i, 127);
        }
    }
    
    void ExitCombat()
    {
        // 战斗结束：淡出音乐
        player.SetFadeTime(0, 2.0);  // 2秒淡出
    }
};
```

#### 场景5：多轨导出工具

```cpp
class MIDIExporter
{
public:
    void ExportToWAV(const os_char *midi_file, const os_char *output_dir)
    {
        MIDIPlayer player;
        player.Load(midi_file);
        
        int channels = player.GetChannelCount();
        int sample_rate = 44100;
        int duration = (int)(player.GetTotalTime() * sample_rate);
        
        // 为每个通道分配缓冲区
        int16_t **buffers = new int16_t*[channels];
        for (int i = 0; i < channels; i++)
        {
            buffers[i] = new int16_t[duration * 2];  // 立体声
        }
        
        // 解码所有通道
        player.DecodeAllChannels(buffers, duration);
        
        // 保存每个通道为独立WAV文件
        for (int i = 0; i < channels; i++)
        {
            MidiChannelInfo info;
            player.GetChannelInfo(i, &info);
            
            char filename[256];
            sprintf(filename, "%s/channel_%02d_%s.wav", 
                   output_dir, i, info.name);
            
            SaveWAV(filename, buffers[i], duration * 2, sample_rate);
            
            delete[] buffers[i];
        }
        
        delete[] buffers;
    }
    
private:
    void SaveWAV(const char *filename, int16_t *data, int samples, int rate)
    {
        // WAV文件保存实现...
    }
};
```

### API 参考

#### 播放控制

```cpp
bool Load(const os_char *filename);              // 加载MIDI文件
bool Load(io::InputStream *stream, int size);    // 从流加载
void Play(bool loop = true);                     // 开始播放
void Stop();                                     // 停止播放
void Pause();                                    // 暂停
void Resume();                                   // 继续
void Clear();                                    // 清除数据
```

#### 状态查询

```cpp
MIDIPlayState GetPlayState() const;              // 获取播放状态
double GetTotalTime() const;                     // 获取总时长
PreciseTime GetPlayTime();                       // 获取当前播放时间
bool IsLoop();                                   // 是否循环
int GetSourceState() const;                      // 获取音源状态
```

#### 音频属性

```cpp
void SetGain(float val);                         // 设置音量 (0.0-1.0)
float GetGain() const;                           // 获取音量
void SetPitch(float val);                        // 设置音高 (0.5-2.0)
float GetPitch() const;                          // 获取音高
void SetLoop(bool val);                          // 设置循环
void SetFadeTime(PreciseTime in, PreciseTime out); // 设置淡入淡出
void AutoGain(float target, PreciseTime duration, const PreciseTime cur); // 自动音量过渡
```

#### 空间音频

```cpp
void SetPosition(const Vector3f &pos);           // 设置3D位置
const Vector3f& GetPosition() const;             // 获取位置
void SetVelocity(const Vector3f &vel);           // 设置速度（多普勒效应）
const Vector3f& GetVelocity() const;             // 获取速度
void SetDirection(const Vector3f &dir);          // 设置方向
const Vector3f& GetDirection() const;            // 获取方向
```

#### MIDI配置

```cpp
bool SetSoundFont(const char *path);             // 设置SoundFont
const char* GetSoundFont() const;                // 获取SoundFont路径
bool SetBank(int bank_id);                       // 设置音色库ID
int GetBank() const;                             // 获取音色库ID
bool SetBankFile(const char *path);              // 设置音色库文件
const char* GetBankFile() const;                 // 获取音色库文件路径
bool SetSampleRate(int rate);                    // 设置采样率
int GetSampleRate() const;                       // 获取采样率
```

#### 通道控制

```cpp
int GetChannelCount() const;                     // 获取通道数量
bool GetChannelInfo(int channel, MidiChannelInfo *info); // 获取通道信息
bool SetChannelProgram(int channel, int program); // 设置乐器
bool SetChannelVolume(int channel, int volume);  // 设置音量 (0-127)
bool SetChannelPan(int channel, int pan);        // 设置声像 (0-127)
bool SetChannelMute(int channel, bool mute);     // 静音/取消静音
bool SetChannelSolo(int channel, bool solo);     // 独奏/取消独奏
bool ResetChannel(int channel);                  // 重置通道
bool ResetAllChannels();                         // 重置所有通道
bool GetChannelActivity(int channel) const;      // 获取通道活跃状态
```

#### 高级功能

```cpp
int DecodeChannel(int channel, int16_t **buffer, int samples);        // 单通道解码
int DecodeAllChannels(int16_t **buffers, int samples);                // 所有通道解码
```

### 性能建议

1. **音色库选择**
   - FluidSynth: 最佳音质，但CPU/内存占用高
   - TinySoundFont: 良好音质，零依赖，适合跨平台
   - OPNMIDI/ADLMIDI: 低CPU，适合复古音色

2. **通道控制**
   - 实时通道控制（音量、静音等）性能开销很小
   - 单通道解码和多通道解码是耗时操作，适合离线处理

3. **内存管理**
   - MIDI文件本身很小（通常<1MB）
   - 主要内存占用来自音色库（SoundFont可达50MB+）

4. **线程安全**
   - 播放控制方法是线程安全的
   - 通道控制方法在播放时调用是安全的

### 支持的MIDI插件

MIDIPlayer 支持所有6个MIDI插件，自动使用已安装的插件：

1. **FluidSynth** - 专业级合成器
2. **TinySoundFont** - 零依赖，易于部署
3. **WildMIDI** - 通用MIDI播放
4. **Libtimidity** - 轻量级
5. **OPNMIDI** - Sega Genesis/Mega Drive 音色
6. **ADLMIDI** - DOS/AdLib/Sound Blaster 音色

### 故障排除

#### 问题：无法加载MIDI文件

**解决方案：**
```cpp
if (!player.Load("music.mid"))
{
    // 检查文件是否存在
    // 检查MIDI插件是否正确安装
    // 查看日志输出
}
```

#### 问题：无法更换乐器

**可能原因：**
- 插件不支持实时Program Change
- 需要在加载文件前配置

**解决方案：**
```cpp
// 确保在Load之前设置配置
player.SetSoundFont("/path/to/soundfont.sf2");
player.Load("music.mid");
```

#### 问题：通道控制不起作用

**检查：**
```cpp
// 确认插件支持通道控制
if (player.GetChannelCount() > 0)
{
    // 支持通道控制
    player.SetChannelVolume(0, 100);
}
else
{
    // 当前插件不支持通道控制
}
```

---

## English Version

### Introduction

`MIDIPlayer` is a professional player class designed specifically for MIDI files, providing complete MIDI control functionality. Unlike the general-purpose `AudioPlayer`, `MIDIPlayer` focuses on MIDI-specific features such as channel control, instrument changes, and soundbank selection.

### Core Features

- ✅ **Real-time Channel Control** - Dynamically adjust volume, pan, mute, and solo for each channel
- ✅ **Dynamic Instrument Changes** - Change channel instruments at runtime (Program Change)
- ✅ **Soundbank Management** - Support for SoundFont (.sf2) and FM chip banks (.wopn/.wopl)
- ✅ **Channel Information Query** - Get detailed status information for each channel
- ✅ **Multi-channel Separation Decode** - Export individual channels as PCM data
- ✅ **Independent Playback Thread** - Non-blocking main thread
- ✅ **Spatial Audio Support** - 3D positioning, velocity, direction, etc.
- ✅ **Fade In/Out** - Automatic volume transitions
- ✅ **Loop Playback** - Seamless looping

### Differences from AudioPlayer

| Feature | AudioPlayer | MIDIPlayer |
|---------|-------------|------------|
| **Supported Formats** | OGG, Opus, MIDI, etc. | MIDI only |
| **Channel Control** | ❌ Not supported | ✅ Full support |
| **Instrument Changes** | ❌ Not supported | ✅ Supported |
| **Soundbank Config** | ❌ Not supported | ✅ Supported |
| **Channel Separation Export** | ❌ Not supported | ✅ Supported |
| **Professional MIDI Features** | ❌ Basic playback | ✅ Full control |
| **Use Cases** | General audio playback | Professional MIDI applications |

### Quick Start

#### 1. Basic Playback

```cpp
#include <hgl/audio/MIDIPlayer.h>

// Create player
MIDIPlayer player;

// Load MIDI file
if (player.Load("music.mid"))
{
    // Start playback (looping)
    player.Play(true);
}
```

#### 2. Configure Soundbank

```cpp
// When using FluidSynth or TinySoundFont
player.SetSoundFont("/usr/share/sounds/sf2/FluidR3_GM.sf2");

// When using OPNMIDI (Sega Genesis sound)
player.SetBank(74);  // Sonic 3 soundbank

// When using ADLMIDI (DOS game sound)
player.SetBank(0);   // DMX OP2 (Doom sound)
```

#### 3. Channel Control

```cpp
// Get channel count
int count = player.GetChannelCount();  // Usually 16

// Query channel information
MidiChannelInfo info;
if (player.GetChannelInfo(0, &info))
{
    printf("Channel 0: Program %d, Volume %d, Pan %d\n",
           info.program, info.volume, info.pan);
}

// Change channel instrument
player.SetChannelProgram(0, 40);  // Change channel 0 to violin (40)

// Adjust channel volume
player.SetChannelVolume(0, 100);  // Volume 0-127

// Set pan
player.SetChannelPan(0, 0);      // 0=left, 64=center, 127=right

// Mute drums
player.SetChannelMute(9, true);

// Solo piano channel
player.SetChannelSolo(0, true);
```

### Use Case Examples

[Full examples similar to Chinese version above...]

### API Reference

[Complete API documentation similar to Chinese version above...]

### Performance Tips

1. **Soundbank Selection**
   - FluidSynth: Best quality, but high CPU/memory usage
   - TinySoundFont: Good quality, zero dependencies, cross-platform
   - OPNMIDI/ADLMIDI: Low CPU, suitable for retro sounds

2. **Channel Control**
   - Real-time channel control (volume, mute, etc.) has minimal overhead
   - Single/multi-channel decode operations are expensive, suitable for offline processing

3. **Memory Management**
   - MIDI files themselves are small (usually <1MB)
   - Main memory usage comes from soundbanks (SoundFonts can be 50MB+)

4. **Thread Safety**
   - Playback control methods are thread-safe
   - Channel control methods are safe to call during playback

### Supported MIDI Plugins

MIDIPlayer supports all 6 MIDI plugins, automatically using installed plugins:

1. **FluidSynth** - Professional synthesizer
2. **TinySoundFont** - Zero dependencies, easy deployment
3. **WildMIDI** - General MIDI playback
4. **Libtimidity** - Lightweight
5. **OPNMIDI** - Sega Genesis/Mega Drive sound
6. **ADLMIDI** - DOS/AdLib/Sound Blaster sound

### Troubleshooting

[Similar troubleshooting section as Chinese version...]

---

## License

This code is part of the CMAudio library and follows the same license as the main repository.
