# FluidSynth Plugin 使用示例 / Usage Examples

## 基本说明 / Basic Information

FluidSynth 插件提供与其他 MIDI 插件相同的接口。所有 MIDI 插件使用相同的插件名称 "MIDI"，使用方法完全一致。

The FluidSynth plugin provides the same interface as other MIDI plugins. All MIDI plugins use the same plugin name "MIDI" with identical usage.

## 三个 MIDI 插件对比 / Comparison of Three MIDI Plugins

### FluidSynth (本插件 / This Plugin)
- ✅ 最佳音质 - 专业级软件合成器
- ✅ 丰富效果 - 内置混响、合唱
- ✅ 高复音数 - 256+ 音符同时发声
- ⚠️ CPU 占用较高
- ⚠️ 需要 SoundFont 文件

### WildMIDI
- ✅ 良好音质
- ✅ 活跃维护
- ✅ 中等资源占用
- ⚠️ 无内置效果

### Libtimidity
- ✅ 轻量级
- ✅ 最快初始化
- ✅ 最低资源占用
- ⚠️ 基础音质

## 基本用法 / Basic Usage

### 示例 1: 使用 AudioPlayer 播放 MIDI 文件 / Example 1: Play MIDI with AudioPlayer

```cpp
#include<hgl/audio/AudioPlayer.h>

// 创建 MIDI 播放器 - 三个插件的代码完全相同
// Create MIDI player - code is identical for all three plugins
AudioPlayer midi_player;

// 加载 MIDI 文件
// Load MIDI file
if (midi_player.Load("orchestral_music.mid"))
{
    // FluidSynth 会提供最佳音质
    // FluidSynth provides the best sound quality
    midi_player.SetGain(0.8f);
    midi_player.Play(true);
}
```

### 示例 2: 专业级音乐播放 / Example 2: Professional Music Playback

```cpp
#include<hgl/audio/AudioPlayer.h>

// FluidSynth 特别适合复杂的交响乐 MIDI
// FluidSynth is ideal for complex orchestral MIDI
AudioPlayer symphony;

if (symphony.Load("beethoven_symphony.mid"))
{
    // 设置淡入效果
    // Set fade in effect
    symphony.SetFadeTime(3.0, 3.0);  // 3秒淡入，3秒淡出
    
    // 播放
    // Play
    symphony.Play(true);
}
```

### 示例 3: 3D 空间音效 / Example 3: 3D Spatial Audio

```cpp
#include<hgl/audio/AudioPlayer.h>
#include<hgl/math/Vector.h>

// 使用 FluidSynth 的高质量合成播放 3D MIDI
// Use FluidSynth's high-quality synthesis for 3D MIDI
AudioPlayer piano_music;

if (piano_music.Load("piano_solo.mid"))
{
    // 设置 3D 位置 - 模拟房间中的钢琴
    // Set 3D position - simulate piano in a room
    piano_music.SetPosition(Vector3f(5.0f, 0.0f, 10.0f));
    
    // 设置距离参数
    // Set distance parameters
    piano_music.SetDistance(2.0f, 50.0f);
    
    piano_music.Play(true);
}
```

### 示例 4: 游戏背景音乐管理器 / Example 4: Game Music Manager

```cpp
#include<hgl/audio/AudioPlayer.h>
#include<map>

class GameMusicManager
{
    std::map<std::string, AudioPlayer*> music_tracks;
    AudioPlayer* current_track;
    
public:
    GameMusicManager() : current_track(nullptr) {}
    
    ~GameMusicManager()
    {
        for (auto& pair : music_tracks)
        {
            delete pair.second;
        }
    }
    
    bool LoadTrack(const std::string& name, const os_char* filename)
    {
        AudioPlayer* player = new AudioPlayer();
        if (player->Load(filename))
        {
            music_tracks[name] = player;
            return true;
        }
        delete player;
        return false;
    }
    
    void PlayTrack(const std::string& name, float fade_time = 2.0)
    {
        auto it = music_tracks.find(name);
        if (it == music_tracks.end())
            return;
        
        // 淡出当前音轨
        // Fade out current track
        if (current_track && current_track->GetPlayState() == PlayState::Play)
        {
            current_track->AutoGain(0.0f, fade_time, GetTimeSec());
            WaitTime(fade_time);
            current_track->Stop();
        }
        
        // 淡入新音轨
        // Fade in new track
        current_track = it->second;
        current_track->SetFadeTime(fade_time, fade_time);
        current_track->Play(true);
    }
};

// 使用示例 / Usage
GameMusicManager music;
music.LoadTrack("menu", "menu_theme.mid");
music.LoadTrack("battle", "battle_music.mid");
music.LoadTrack("victory", "victory_fanfare.mid");

// 在游戏中切换音乐 / Switch music in game
music.PlayTrack("menu");
// ... 进入战斗 / Enter battle
music.PlayTrack("battle");
// ... 胜利 / Victory
music.PlayTrack("victory");
```

## 配置 / Configuration

### SoundFont 配置 / SoundFont Configuration

FluidSynth 的音质取决于 SoundFont 文件：

FluidSynth's sound quality depends on the SoundFont file:

```bash
# 安装默认 SoundFont
# Install default SoundFont
sudo apt-get install fluid-soundfont-gm

# 或使用高质量 SoundFont
# Or use high-quality SoundFont
export FLUIDSYNTH_SF2=/usr/share/sounds/sf2/FluidR3_GM.sf2

# 使用 GeneralUser GS (更高质量)
# Use GeneralUser GS (higher quality)
export FLUIDSYNTH_SF2=/path/to/GeneralUser_GS.sf2
```

### 推荐的 SoundFont / Recommended SoundFonts

1. **FluidR3_GM** (默认，~150MB)
   - 包含在 fluid-soundfont-gm 包中
   - 平衡的质量和大小

2. **GeneralUser GS** (~30MB)
   - 高质量，文件较小
   - 适合大多数场景

3. **Timbres of Heaven** (~350MB)
   - 最高质量
   - 适合专业音乐制作

## 性能优化 / Performance Optimization

### 示例 5: 为资源受限环境优化 / Example 5: Optimize for Resource-Constrained Environments

如果遇到性能问题，可以在 MidiRead.cpp 中调整设置：

If you encounter performance issues, adjust settings in MidiRead.cpp:

```cpp
// 降低复音数
// Reduce polyphony
fluid_settings_setint(fluid_settings, "synth.polyphony", 128);  // 默认 256

// 禁用效果以节省 CPU
// Disable effects to save CPU
fluid_settings_setint(fluid_settings, "synth.reverb.active", 0);
fluid_settings_setint(fluid_settings, "synth.chorus.active", 0);
```

### 示例 6: 动态音量和音调控制 / Example 6: Dynamic Volume and Pitch Control

```cpp
#include<hgl/audio/AudioPlayer.h>

AudioPlayer game_music;
game_music.Load("game_theme.mid");
game_music.Play(true);

// 根据游戏状态调整音量
// Adjust volume based on game state
void OnEnterMenu()
{
    game_music.AutoGain(0.5f, 2.0, GetTimeSec());  // 降低音量
}

void OnEnterBattle()
{
    game_music.AutoGain(1.0f, 1.0, GetTimeSec());  // 提高音量
}

// 根据游戏速度调整音调
// Adjust pitch based on game speed
void SetGameSpeed(float speed)
{
    game_music.SetPitch(speed);  // 1.0 = 正常, 1.5 = 快速, 0.75 = 慢速
}
```

## 多 MIDI 插件共存 / Multiple MIDI Plugin Coexistence

### 示例 7: 检查当前使用的 MIDI 插件 / Example 7: Check Active MIDI Plugin

虽然所有插件使用相同的名称，但可以通过测试文件来验证：

Although all plugins use the same name, you can verify by testing:

```cpp
#include<hgl/audio/AudioPlayer.h>

void TestMIDIPlugin()
{
    AudioPlayer test;
    
    // 尝试加载测试 MIDI 文件
    // Try loading a test MIDI file
    if (test.Load("test.mid"))
    {
        // 根据音质判断使用的插件
        // Judge which plugin by sound quality
        // FluidSynth: 最佳音质，内置混响
        // WildMIDI: 良好音质
        // Libtimidity: 基础音质
        
        test.Play(false);
        WaitTime(5.0);  // 播放 5 秒测试
        test.Stop();
    }
}
```

### 强制使用特定插件 / Force Specific Plugin

要确保使用 FluidSynth，只安装 libfluidsynth-dev：

To ensure FluidSynth is used, only install libfluidsynth-dev:

```bash
# 仅安装 FluidSynth
# Install only FluidSynth
sudo apt-get install libfluidsynth-dev fluid-soundfont-gm

# 不安装其他 MIDI 库
# Don't install other MIDI libraries
# sudo apt-get remove libwildmidi-dev libtimidity-dev
```

## 故障排除 / Troubleshooting

### 问题：音质不佳
**解决方案：**
```bash
# 使用更高质量的 SoundFont
# Use higher quality SoundFont
wget http://www.schristiancollins.com/generaluser/GeneralUser_GS_1.471.zip
unzip GeneralUser_GS_1.471.zip
export FLUIDSYNTH_SF2=/path/to/GeneralUser_GS.sf2
```

### 问题：CPU 占用过高
**解决方案：**
- 在代码中降低复音数（128 或更低）
- 禁用混响和合唱效果
- 使用更简单的 SoundFont 文件

### 问题：初始化失败
**解决方案：**
```bash
# 检查 SoundFont 文件是否存在
ls -l /usr/share/sounds/sf2/FluidR3_GM.sf2

# 重新安装 SoundFont
sudo apt-get install --reinstall fluid-soundfont-gm

# 检查 FluidSynth 版本
fluidsynth --version
```

### 问题：某些音符被截断
**解决方案：**
- 增加复音数设置（256 或更高）
- 检查 MIDI 文件是否过于复杂

## 高级示例 / Advanced Examples

### 示例 8: 多轨道音乐系统 / Example 8: Multi-Track Music System

```cpp
#include<hgl/audio/AudioPlayer.h>
#include<vector>

class MultiTrackMusicSystem
{
    std::vector<AudioPlayer*> tracks;
    
public:
    ~MultiTrackMusicSystem()
    {
        for (auto track : tracks)
        {
            delete track;
        }
    }
    
    void AddTrack(const os_char* filename, float volume = 1.0f)
    {
        AudioPlayer* player = new AudioPlayer();
        if (player->Load(filename))
        {
            player->SetGain(volume);
            tracks.push_back(player);
        }
        else
        {
            delete player;
        }
    }
    
    void PlayAll()
    {
        for (auto track : tracks)
        {
            track->Play(true);
        }
    }
    
    void StopAll()
    {
        for (auto track : tracks)
        {
            track->Stop();
        }
    }
    
    void SetTrackVolume(size_t index, float volume)
    {
        if (index < tracks.size())
        {
            tracks[index]->SetGain(volume);
        }
    }
};

// 使用多轨系统创建分层音乐
// Use multi-track system for layered music
MultiTrackMusicSystem music;
music.AddTrack("drums.mid", 0.8f);
music.AddTrack("bass.mid", 0.7f);
music.AddTrack("melody.mid", 1.0f);
music.PlayAll();
```

## 相关文档 / Related Documentation

- [AudioPlayer 分析文档](../../AudioPlayer_Analysis.md)
- [快速参考指南](../../QUICK_REFERENCE.md)
- [FluidSynth 插件 README](README.md)
- [WildMIDI 插件文档](../Audio.WildMidi/README.md)
- [Libtimidity 插件文档](../Audio.Timidity/README.md)

## 总结 / Summary

FluidSynth 插件提供：
- ✅ 三个 MIDI 插件中的最佳音质
- ✅ 专业级软件合成
- ✅ 内置音频效果（混响、合唱）
- ✅ 高复音数支持
- ⚠️ 需要更多 CPU 和内存资源
- ⚠️ 需要 SoundFont 文件

适合对音质有高要求的音乐应用和游戏。

The FluidSynth plugin provides:
- ✅ Best sound quality among three MIDI plugins
- ✅ Professional-grade software synthesis
- ✅ Built-in audio effects (reverb, chorus)
- ✅ High polyphony support
- ⚠️ Requires more CPU and memory resources
- ⚠️ Requires SoundFont files

Ideal for music applications and games with high sound quality requirements.
