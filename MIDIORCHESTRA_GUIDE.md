# MIDIOrchestraPlayer使用指南 / MIDIOrchestraPlayer Usage Guide

## 概述 / Overview

**中文：**
`MIDIOrchestraPlayer` 是一个专业的3D空间MIDI播放器，可以将MIDI文件的每个通道独立解码并分配到不同的3D空间位置，模拟真实乐队或交响乐团的空间布局。这为VR音乐厅、游戏、音乐教育软件等应用提供了沉浸式的音频体验。

**English:**
`MIDIOrchestraPlayer` is a professional 3D spatial MIDI player that independently decodes each MIDI channel and assigns them to different 3D spatial positions, simulating the spatial layout of a real orchestra or band. This provides an immersive audio experience for VR concert halls, games, music education software, and other applications.

## 核心特性 / Key Features

**中文：**
- ✅ 16个独立的AudioSource（每个MIDI通道一个）
- ✅ 完美同步的多通道播放
- ✅ 4种预设布局：标准交响乐、室内乐、爵士乐、摇滚乐
- ✅ 自定义通道3D位置
- ✅ 实时位置调整和音量控制
- ✅ 独立的距离衰减和空间音频效果
- ✅ 支持乐队中心位置和整体缩放
- ✅ 完整的MIDI控制（乐器更换、静音、独奏）

**English:**
- ✅ 16 independent AudioSources (one per MIDI channel)
- ✅ Perfectly synchronized multi-channel playback
- ✅ 4 preset layouts: Standard Symphony, Chamber, Jazz, Rock
- ✅ Custom channel 3D positioning
- ✅ Real-time position adjustment and volume control
- ✅ Independent distance attenuation and spatial audio effects
- ✅ Support for orchestra center position and overall scaling
- ✅ Complete MIDI control (instrument changes, mute, solo)

## 快速开始 / Quick Start

```cpp
#include <hgl/audio/MIDIOrchestraPlayer.h>

// 创建播放器 / Create player
MIDIOrchestraPlayer orchestra;

// 加载MIDI文件 / Load MIDI file
orchestra.Load("symphony.mid");

// 设置标准交响乐团布局 / Set standard symphony layout
orchestra.SetLayout(OrchestraLayout::Standard);

// 设置乐队中心位置（在听众前方5米）/ Set orchestra center (5m in front of listener)
orchestra.SetOrchestraCenter(Vector3f(0, 0, 5));

// 开始播放 / Start playback
orchestra.Play(true);  // true = loop
```

## 预设布局 / Preset Layouts

### 1. 标准交响乐团布局 / Standard Symphony Layout

**中文：** 模拟标准交响乐团的舞台布局，适合古典音乐。

**English:** Simulates standard symphony orchestra stage layout, suitable for classical music.

**通道位置 / Channel Positions:**
- Channel 0: Piano (钢琴) - 舞台中央前方
- Channel 1: Violin I (小提琴组I) - 左前方
- Channel 2: Violin II (小提琴组II) - 右前方
- Channel 3: Viola (中提琴) - 左中
- Channel 4: Cello (大提琴) - 右中
- Channel 5: Bass (低音提琴) - 中后方
- Channel 9: Drums (鼓组) - 右中

```cpp
orchestra.SetLayout(OrchestraLayout::Standard);
```

### 2. 室内乐布局 / Chamber Music Layout

**中文：** 更紧凑的布局，适合小型室内乐演奏。

**English:** More compact layout for small chamber music ensembles.

```cpp
orchestra.SetLayout(OrchestraLayout::Chamber);
```

### 3. 爵士乐布局 / Jazz Ensemble Layout

**中文：** 典型的爵士乐队布局，钢琴居中，萨克斯和小号分左右。

**English:** Typical jazz ensemble layout with piano center, saxophone and trumpet left/right.

```cpp
orchestra.SetLayout(OrchestraLayout::Jazz);
```

### 4. 摇滚乐队布局 / Rock Band Layout

**中文：** 摇滚乐队布局，主音吉他、节奏吉他、贝斯、鼓组分布。

**English:** Rock band layout with lead guitar, rhythm guitar, bass, drums distribution.

```cpp
orchestra.SetLayout(OrchestraLayout::Rock);
```

## 示例 1: VR音乐厅体验 / Example 1: VR Concert Hall Experience

**中文：** 创建沉浸式VR音乐厅，用户可以在虚拟音乐厅中自由移动。

**English:** Create an immersive VR concert hall where users can move freely in the virtual venue.

```cpp
#include <hgl/audio/MIDIOrchestraPlayer.h>
#include <hgl/audio/Listener.h>

class VRConcertHall
{
    MIDIOrchestraPlayer orchestra;
    AudioListener *listener;

public:
    void Initialize()
    {
        // 加载交响乐 / Load symphony
        orchestra.Load("beethoven_symphony_5.mid");
        
        // 设置标准交响乐团布局 / Set standard symphony layout
        orchestra.SetLayout(OrchestraLayout::Standard);
        
        // 乐队在舞台上，距离观众20米 / Orchestra on stage, 20m from audience
        orchestra.SetOrchestraCenter(Vector3f(0, 0, 20));
        
        // 放大乐队规模（更宽的舞台）/ Scale up orchestra (wider stage)
        orchestra.SetOrchestraScale(1.5f);
        
        // 开始播放 / Start playback
        orchestra.Play(true);
    }

    // VR中更新听众位置 / Update listener position in VR
    void UpdateListener Position(const Vector3f &position, const Vector3f &forward, const Vector3f &up)
    {
        if(listener)
        {
            listener->SetPosition(position);
            listener->SetOrientation(forward, up);
        }
    }

    // 用户走近某个乐器 / User walks close to an instrument
    void HighlightInstrument(int channel)
    {
        // 提高该通道音量 / Increase channel volume
        orchestra.SetChannelVolume(channel, 1.2f);
        
        // 降低其他通道音量 / Decrease other channel volumes
        for(int i=0; i<16; i++)
        {
            if(i != channel)
                orchestra.SetChannelVolume(i, 0.6f);
        }
    }
};
```

## 示例 2: 游戏中的动态乐队 / Example 2: Dynamic Orchestra in Game

**中文：** 游戏中的乐队表演，根据游戏状态动态调整。

**English:** Orchestra performance in game, dynamically adjusted based on game state.

```cpp
class GameOrchestra
{
    MIDIOrchestraPlayer orchestra;
    
    enum GameState {
        Peaceful,
        Tense,
        Combat,
        Victory
    };

public:
    void Initialize()
    {
        orchestra.Load("game_music.mid");
        orchestra.SetLayout(OrchestraLayout::Standard);
        orchestra.SetOrchestraCenter(Vector3f(0, 2, 10));  // 乐队在玩家上方前方
        orchestra.Play(true);
    }

    // 根据游戏状态调整音乐 / Adjust music based on game state
    void SetGameState(GameState state)
    {
        switch(state)
        {
            case Peaceful:
                // 平静：只有弦乐和长笛 / Peaceful: Only strings and flute
                orchestra.MuteChannel(9, true);   // 静音鼓组 / Mute drums
                orchestra.MuteChannel(10, true);  // 静音小号 / Mute trumpet
                orchestra.MuteChannel(11, true);  // 静音长号 / Mute trombone
                orchestra.SetChannelVolume(1, 0.8f);  // 小提琴 / Violin
                orchestra.SetChannelVolume(6, 0.6f);  // 长笛 / Flute
                break;

            case Tense:
                // 紧张：加入低音和定音鼓 / Tense: Add bass and timpani
                orchestra.MuteChannel(9, false);
                orchestra.SetChannelVolume(5, 1.0f);   // 低音提琴 / Bass
                orchestra.SetChannelVolume(15, 0.7f);  // 定音鼓 / Timpani
                break;

            case Combat:
                // 战斗：所有乐器全开 / Combat: All instruments
                for(int i=0; i<16; i++)
                    orchestra.MuteChannel(i, false);
                orchestra.SetChannelVolume(9, 1.2f);   // 鼓组加强 / Drums amplified
                orchestra.SetChannelVolume(10, 1.0f);  // 小号 / Trumpet
                break;

            case Victory:
                // 胜利：欢快的旋律，突出小号和长笛 / Victory: Joyful, highlight trumpet and flute
                orchestra.SoloChannel(10, false);  // 取消独奏 / Cancel solo
                orchestra.SetChannelVolume(10, 1.2f);  // 小号 / Trumpet
                orchestra.SetChannelVolume(6, 1.0f);   // 长笛 / Flute
                orchestra.FadeIn(2.0f);  // 2秒淡入 / 2s fade in
                break;
        }
    }

    // Boss战：特殊的音乐强化 / Boss battle: Special music enhancement
    void BossBattleMode()
    {
        // 将乐队移到更近的位置，增加冲击力 / Move orchestra closer for impact
        orchestra.SetOrchestraCenter(Vector3f(0, 0, 5));
        
        // 放大乐队规模 / Scale up orchestra
        orchestra.SetOrchestraScale(2.0f);
        
        // 所有乐器最大音量 / All instruments max volume
        for(int i=0; i<16; i++)
            orchestra.SetChannelVolume(i, 1.5f);
    }
};
```

## 示例 3: 音乐教育软件 / Example 3: Music Education Software

**中文：** 让学生可以听到每个乐器的独立演奏，理解乐器在乐队中的位置。

**English:** Let students hear each instrument independently and understand their position in the orchestra.

```cpp
class MusicLearningTool
{
    MIDIOrchestraPlayer orchestra;
    int currentInstrument = 0;

public:
    void Initialize()
    {
        orchestra.Load("educational_piece.mid");
        orchestra.SetLayout(OrchestraLayout::Standard);
        orchestra.SetOrchestraCenter(Vector3f(0, 0, 8));
        orchestra.Play(true);
    }

    // 突出显示特定乐器 / Highlight specific instrument
    void HighlightInstrument(int channel)
    {
        currentInstrument = channel;
        
        // 独奏该乐器 / Solo this instrument
        orchestra.SoloChannel(channel, true);
        
        // 将该乐器移到中央前方，方便听清 / Move instrument to center front
        orchestra.SetChannelPosition(channel, Vector3f(0, 0, -2));
        
        // 提高音量 / Increase volume
        orchestra.SetChannelVolume(channel, 1.5f);
    }

    // 显示所有乐器 / Show all instruments
    void ShowAllInstruments()
    {
        // 取消独奏 / Cancel solo
        orchestra.SoloChannel(currentInstrument, false);
        
        // 恢复原始布局 / Restore original layout
        orchestra.SetLayout(OrchestraLayout::Standard);
    }

    // 显示乐器信息 / Display instrument info
    void ShowInstrumentInfo(int channel)
    {
        MidiChannelInfo info = orchestra.GetChannelInfo(channel);
        Vector3f pos = orchestra.GetChannelPosition(channel);
        
        printf("通道 %d / Channel %d:\n", channel, channel);
        printf("乐器 / Instrument: %s\n", info.name);
        printf("音量 / Volume: %d/127\n", info.volume);
        printf("声像 / Pan: %d (0=左,64=中,127=右)\n", info.pan);
        printf("位置 / Position: (%.2f, %.2f, %.2f)\n", pos.x, pos.y, pos.z);
        printf("活跃音符 / Active notes: %d\n", info.note_count);
    }

    // 练习模式：逐个学习乐器 / Practice mode: Learn instruments one by one
    void PracticeMode()
    {
        for(int ch=0; ch<orchestra.GetChannelCount(); ch++)
        {
            printf("\n=== 学习通道 %d / Learning Channel %d ===\n", ch, ch);
            
            ShowInstrumentInfo(ch);
            HighlightInstrument(ch);
            
            // 播放5秒让学生听 / Play for 5 seconds for student to listen
            WaitSeconds(5);
            
            ShowAllInstruments();
            WaitSeconds(2);
        }
    }
};
```

## 示例 4: 自定义乐队布局 / Example 4: Custom Orchestra Layout

**中文：** 创建自定义的3D布局，完全控制每个乐器的位置。

**English:** Create a custom 3D layout with full control over each instrument position.

```cpp
void CreateCustomLayout()
{
    MIDIOrchestraPlayer orchestra;
    orchestra.Load("custom_music.mid");
    
    // 设置为自定义布局模式 / Set to custom layout mode
    orchestra.SetLayout(OrchestraLayout::Custom);
    
    // 圆形布局：乐器环绕听众 / Circular layout: Instruments surround listener
    float radius = 5.0f;  // 5米半径 / 5m radius
    int channelCount = orchestra.GetChannelCount();
    
    for(int i=0; i<channelCount; i++)
    {
        float angle = (float)i / channelCount * 2.0f * 3.14159f;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        
        orchestra.SetChannelPosition(i, Vector3f(x, 0, z));
        orchestra.SetChannelVolume(i, 1.0f);
    }
    
    orchestra.Play(true);
}

// 立体环绕：不同高度的乐器 / Stereo surround: Instruments at different heights
void CreateVerticalLayout()
{
    MIDIOrchestraPlayer orchestra;
    orchestra.Load("spatial_music.mid");
    
    orchestra.SetLayout(OrchestraLayout::Custom);
    
    // 低音乐器在底层 / Bass instruments at bottom
    orchestra.SetChannelPosition(4, Vector3f(-2, -1, 5));  // Cello低
    orchestra.SetChannelPosition(5, Vector3f(2, -1, 5));   // Bass低
    
    // 中音乐器在中层 / Mid-range instruments at middle
    orchestra.SetChannelPosition(1, Vector3f(-3, 0, 3));  // Violin I中
    orchestra.SetChannelPosition(2, Vector3f(3, 0, 3));   // Violin II中
    orchestra.SetChannelPosition(3, Vector3f(0, 0, 4));   // Viola中
    
    // 高音乐器在上层 / High-pitch instruments at top
    orchestra.SetChannelPosition(6, Vector3f(-2, 1, 2));  // Flute高
    orchestra.SetChannelPosition(7, Vector3f(2, 1, 2));   // Oboe高
    orchestra.SetChannelPosition(10, Vector3f(0, 1, 3));  // Trumpet高
    
    orchestra.Play(true);
}
```

## 示例 5: 实时交互控制 / Example 5: Real-time Interactive Control

**中文：** 允许用户实时调整乐队参数。

**English:** Allow users to adjust orchestra parameters in real-time.

```cpp
class InteractiveOrchestra
{
    MIDIOrchestraPlayer orchestra;

public:
    void Initialize()
    {
        orchestra.Load("interactive_music.mid");
        orchestra.SetLayout(OrchestraLayout::Standard);
        orchestra.Play(true);
    }

    // 用鼠标或VR手柄拖动乐器位置 / Drag instrument position with mouse or VR controller
    void DragInstrument(int channel, const Vector3f &newPosition)
    {
        orchestra.SetChannelPosition(channel, newPosition);
        
        // 视觉反馈：显示乐器图标在新位置 / Visual feedback: Show instrument icon at new position
        ShowInstrumentIcon(channel, newPosition);
    }

    // 滑块控制音量 / Slider controls volume
    void OnVolumeSlider(int channel, float volume)
    {
        orchestra.SetChannelVolume(channel, volume);
    }

    // 按钮控制静音 / Button controls mute
    void OnMuteButton(int channel, bool mute)
    {
        orchestra.MuteChannel(channel, mute);
    }

    // 按钮控制独奏 / Button controls solo
    void OnSoloButton(int channel, bool solo)
    {
        orchestra.SoloChannel(channel, solo);
    }

    // 改变乐器 / Change instrument
    void OnInstrumentChange(int channel, int program)
    {
        orchestra.SetChannelProgram(channel, program);
    }

    // 预设按钮 / Preset buttons
    void OnLayoutButton(int layoutIndex)
    {
        static const OrchestraLayout layouts[] = {
            OrchestraLayout::Standard,
            OrchestraLayout::Chamber,
            OrchestraLayout::Jazz,
            OrchestraLayout::Rock
        };
        
        if(layoutIndex >= 0 && layoutIndex < 4)
        {
            orchestra.SetLayout(layouts[layoutIndex]);
        }
    }

    // 缩放控制 / Scale control
    void OnScaleSlider(float scale)
    {
        orchestra.SetOrchestraScale(scale);
    }

    // 位置控制 / Position control
    void OnPositionChange(const Vector3f &center)
    {
        orchestra.SetOrchestraCenter(center);
    }
};
```

## API参考 / API Reference

### 基本控制 / Basic Control

```cpp
// 加载MIDI文件 / Load MIDI file
bool Load(const os_char *filename);
bool Load(io::InputStream *stream, int size=-1);

// 播放控制 / Playback control
void Play(bool loop=false);    // 开始播放 / Start playback
void Pause();                  // 暂停 / Pause
void Resume();                 // 继续 / Resume
void Stop();                   // 停止 / Stop
void Close();                  // 关闭 / Close
```

### 布局控制 / Layout Control

```cpp
// 设置预设布局 / Set preset layout
void SetLayout(OrchestraLayout layout);

// 设置乐队中心位置 / Set orchestra center position
void SetOrchestraCenter(const Vector3f &center);

// 设置乐队缩放 / Set orchestra scale
void SetOrchestraScale(float scale);

// 设置单个通道位置 / Set individual channel position
void SetChannelPosition(int channel, const Vector3f &position);

// 获取通道位置 / Get channel position
Vector3f GetChannelPosition(int channel) const;
```

### 通道控制 / Channel Control

```cpp
// 设置通道音量 / Set channel volume
void SetChannelVolume(int channel, float volume);

// 启用/禁用通道 / Enable/disable channel
void SetChannelEnabled(int channel, bool enabled);

// 获取通道AudioSource / Get channel AudioSource
AudioSource* GetChannelSource(int channel);

// 静音通道 / Mute channel
void MuteChannel(int channel, bool mute);

// 独奏通道 / Solo channel
void SoloChannel(int channel, bool solo);

// 设置通道乐器 / Set channel instrument
void SetChannelProgram(int channel, int program);
```

### 信息查询 / Information Query

```cpp
// 获取通道数量 / Get channel count
const int GetChannelCount();

// 获取通道信息 / Get channel info
const MidiChannelInfo GetChannelInfo(int channel);
```

### 自动增益 / Auto Gain

```cpp
// 自动增益 / Auto gain
void AutoGain(float start_gain, float gap, float end_gain);

// 淡入 / Fade in
void FadeIn(float gap);

// 淡出 / Fade out
void FadeOut(float gap);
```

## 性能考虑 / Performance Considerations

**中文：**
- 16个独立AudioSource会比单一AudioSource消耗更多CPU和内存
- 建议在现代系统上使用（多核CPU，4GB+内存）
- 可以通过SetChannelEnabled禁用不需要的通道以节省资源
- 3D音频计算会增加CPU负担，但OpenAL有硬件加速支持

**English:**
- 16 independent AudioSources consume more CPU and memory than single AudioSource
- Recommended for modern systems (multi-core CPU, 4GB+ RAM)
- Can use SetChannelEnabled to disable unused channels to save resources
- 3D audio calculations increase CPU load, but OpenAL has hardware acceleration support

## 故障排除 / Troubleshooting

**问题：某些通道没有声音 / Problem: Some channels have no sound**

**中文：**
- 检查通道是否被禁用：`SetChannelEnabled(channel, true)`
- 检查通道是否被静音：`MuteChannel(channel, false)`
- 检查MIDI文件该通道是否有音符数据
- 检查通道音量：`SetChannelVolume(channel, 1.0f)`

**English:**
- Check if channel is disabled: `SetChannelEnabled(channel, true)`
- Check if channel is muted: `MuteChannel(channel, false)`
- Check if MIDI file has note data on that channel
- Check channel volume: `SetChannelVolume(channel, 1.0f)`

**问题：通道不同步 / Problem: Channels out of sync**

**中文：**
- 这通常是系统性能问题，尝试减少活跃通道数
- 检查是否有其他程序占用大量CPU
- 尝试增加音频缓冲区大小

**English:**
- This is usually a system performance issue, try reducing active channel count
- Check if other programs are using significant CPU
- Try increasing audio buffer size

**问题：3D定位不明显 / Problem: 3D positioning not obvious**

**中文：**
- 检查听众(Listener)位置是否正确设置
- 尝试增加乐队缩放：`SetOrchestraScale(2.0f)`
- 检查各通道的距离衰减参数
- 确保使用立体声输出设备

**English:**
- Check if Listener position is correctly set
- Try increasing orchestra scale: `SetOrchestraScale(2.0f)`
- Check distance attenuation parameters for each channel
- Ensure using stereo output device

## 总结 / Summary

**中文：**
`MIDIOrchestraPlayer`为MIDI音乐提供了前所未有的3D空间表现力。通过将每个MIDI通道独立解码并赋予3D位置，它能创造出真实乐队演奏的沉浸式体验。无论是VR应用、游戏、还是教育软件，这个播放器都能提供独特的音频维度。

**English:**
`MIDIOrchestraPlayer` provides unprecedented 3D spatial expressiveness for MIDI music. By independently decoding each MIDI channel and assigning 3D positions, it can create an immersive experience of real orchestra performance. Whether for VR applications, games, or educational software, this player provides a unique audio dimension.

## 相关文档 / Related Documentation

- **MIDIPLAYER_GUIDE.md** - 标准MIDIPlayer使用指南
- **MIDI_CHANNEL_API.md** - MIDI通道控制API文档
- **MIDI_CONFIG_API.md** - MIDI配置API文档
- **AudioPlayer_Analysis.md** - AudioPlayer架构分析
