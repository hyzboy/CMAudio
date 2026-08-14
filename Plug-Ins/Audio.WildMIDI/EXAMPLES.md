# WildMIDI Plugin 使用示例 / Usage Examples

## 基本用法 / Basic Usage

### 示例 1: 使用 AudioPlayer 播放 MIDI 文件 / Example 1: Play MIDI with AudioPlayer

```cpp
#include<hgl/audio/AudioPlayer.h>

// 创建 MIDI 播放器并循环播放
AudioPlayer midi_player;

// 加载 MIDI 文件
if (midi_player.Load("background_music.mid"))
{
    // 设置音量
    midi_player.SetGain(0.8f);
    
    // 设置淡入淡出效果
    midi_player.SetFadeTime(2.0, 2.0);  // 2秒淡入，2秒淡出
    
    // 开始循环播放
    midi_player.Play(true);
}
```

### 示例 2: 使用 AudioBuffer 播放短 MIDI / Example 2: Play Short MIDI with AudioBuffer

```cpp
#include<hgl/audio/AudioBuffer.h>
#include<hgl/audio/AudioSource.h>

// 加载 MIDI 音效
AudioBuffer midi_sfx("sound_effect.midi");

// 创建音源并播放
AudioSource source;
if (source.Link(&midi_sfx))
{
    source.SetGain(1.0f);
    source.Play();
}
```

### 示例 3: 从内存加载 MIDI / Example 3: Load MIDI from Memory

```cpp
#include<hgl/audio/AudioPlayer.h>
#include<hgl/io/MemoryInputStream.h>

// 假设 midi_data 是已加载到内存的 MIDI 数据
unsigned char *midi_data = ...;
int midi_size = ...;

// 创建内存输入流
io::MemoryInputStream stream(midi_data, midi_size);

// 从流中加载
AudioPlayer player;
if (player.Load(&stream, midi_size, AudioFileType::MIDI))
{
    player.Play(true);
}
```

### 示例 4: 播放多个 MIDI 文件序列 / Example 4: Play Multiple MIDI Files in Sequence

```cpp
#include<hgl/audio/AudioPlayer.h>
#include<vector>

class MidiSequence
{
    AudioPlayer player;
    std::vector<OSString> playlist;
    size_t current_index;
    
public:
    MidiSequence()
    {
        current_index = 0;
    }
    
    void AddTrack(const os_char *filename)
    {
        playlist.push_back(filename);
    }
    
    void Play()
    {
        if (playlist.empty())
            return;
            
        PlayTrack(0);
    }
    
    void PlayTrack(size_t index)
    {
        if (index >= playlist.size())
            return;
            
        current_index = index;
        player.Load(playlist[index]);
        player.Play(false);  // 不循环
    }
    
    void Update()
    {
        // 检查当前曲目是否播放完成
        if (player.GetPlayState() == PlayState::None)
        {
            // 播放下一首
            size_t next = current_index + 1;
            if (next < playlist.size())
            {
                PlayTrack(next);
            }
            else
            {
                // 列表播放完成，重新开始
                PlayTrack(0);
            }
        }
    }
};

// 使用示例
MidiSequence sequence;
sequence.AddTrack("track1.mid");
sequence.AddTrack("track2.mid");
sequence.AddTrack("track3.mid");
sequence.Play();

// 在游戏循环中调用
while (running)
{
    sequence.Update();
    // ... 其他游戏逻辑
}
```

### 示例 5: 3D 空间 MIDI 音效 / Example 5: 3D Spatial MIDI Sound

```cpp
#include<hgl/audio/AudioPlayer.h>
#include<hgl/math/Vector.h>

// 创建环境音乐源
AudioPlayer ambient_music;
ambient_music.Load("ambient.mid");

// 设置 3D 位置（例如：在远处的音乐盒）
ambient_music.SetPosition(Vector3f(50.0f, 2.0f, 30.0f));

// 设置距离参数
ambient_music.SetDistance(5.0f, 100.0f);  // 5米内完整音量，100米外听不到

// 设置衰减因子
ambient_music.SetRolloffFactor(1.0f);

// 开始播放
ambient_music.Play(true);

// 在游戏循环中更新监听者位置
// ... 监听者移动时，音量会根据距离自动调整
```

### 示例 6: 动态音量控制 / Example 6: Dynamic Volume Control

```cpp
#include<hgl/audio/AudioPlayer.h>
#include<hgl/time/Time.h>

AudioPlayer battle_music;
battle_music.Load("battle.mid");
battle_music.Play(true);

// 进入战斗时，3秒内音量从0.3渐变到1.0
void OnBattleStart()
{
    battle_music.AutoGain(1.0f, 3.0, GetTimeSec());
}

// 战斗结束时，5秒内音量从当前值渐变到0.3
void OnBattleEnd()
{
    battle_music.AutoGain(0.3f, 5.0, GetTimeSec());
}
```

### 示例 7: 检查 MIDI 支持 / Example 7: Check MIDI Support

```cpp
#include<hgl/audio/AudioFileType.h>

// 检查文件是否为 MIDI 格式
AudioFileType type = CheckAudioFileType("music.mid");
if (type == AudioFileType::MIDI)
{
    // 这是 MIDI 文件
    // 可以使用 AudioPlayer 或 AudioBuffer 加载
}

// 检查扩展名
AudioFileType type2 = CheckAudioExtName(OS_TEXT("midi"));
if (type2 == AudioFileType::MIDI)
{
    // .midi 扩展名被识别为 MIDI 格式
}
```

## 高级用法 / Advanced Usage

### 示例 8: MIDI 播放器类封装 / Example 8: MIDI Player Class Wrapper

```cpp
#include<hgl/audio/AudioPlayer.h>

class MIDIPlayer
{
private:
    AudioPlayer player;
    bool is_playing;
    float default_volume;
    
public:
    MIDIPlayer() : is_playing(false), default_volume(0.7f)
    {
    }
    
    bool LoadFile(const os_char *filename)
    {
        if (player.Load(filename, AudioFileType::MIDI))
        {
            player.SetGain(default_volume);
            return true;
        }
        return false;
    }
    
    void Play(bool loop = true)
    {
        player.Play(loop);
        is_playing = true;
    }
    
    void Stop()
    {
        player.Stop();
        is_playing = false;
    }
    
    void Pause()
    {
        if (is_playing)
        {
            player.Pause();
        }
    }
    
    void Resume()
    {
        if (is_playing)
        {
            player.Resume();
        }
    }
    
    void SetVolume(float volume)
    {
        default_volume = volume;
        player.SetGain(volume);
    }
    
    float GetVolume() const
    {
        return player.GetGain();
    }
    
    void FadeIn(double duration)
    {
        player.AutoGain(default_volume, duration, GetTimeSec());
    }
    
    void FadeOut(double duration)
    {
        player.AutoGain(0.0f, duration, GetTimeSec());
    }
    
    bool IsPlaying() const
    {
        return is_playing && (player.GetPlayState() == PlayState::Play);
    }
    
    double GetPlayTime() const
    {
        return player.GetPlayTime();
    }
    
    double GetTotalTime() const
    {
        return player.GetTotalTime();
    }
};

// 使用封装的播放器
MIDIPlayer midi;
if (midi.LoadFile("soundtrack.mid"))
{
    midi.SetVolume(0.8f);
    midi.FadeIn(2.0);  // 2秒淡入
    midi.Play(true);
}
```

## 性能建议 / Performance Tips

### 适合 MIDI 的场景 / Good Use Cases for MIDI

1. **背景音乐** - MIDI 文件小，适合长时间循环播放
2. **游戏配乐** - 可以根据游戏状态动态调整音量
3. **复古游戏** - MIDI 适合 8-bit/16-bit 风格的游戏音乐
4. **资源受限环境** - MIDI 文件比 MP3/OGG 小得多

### 不适合 MIDI 的场景 / When NOT to Use MIDI

1. **需要人声** - MIDI 不能播放录制的人声
2. **特定音色要求** - MIDI 音色取决于音色库
3. **音质要求极高** - 预渲染的 OGG/OPUS 音质更稳定

### 优化建议 / Optimization Tips

1. **预加载常用 MIDI** - 避免游戏中频繁加载
2. **使用 AudioBuffer 播放短音效** - 不要为每个短音效创建 AudioPlayer
3. **合理设置音量** - MIDI 合成可能比预渲染音频更响亮
4. **配置音色库** - 使用高质量的 SoundFont 可以提升音质

## 故障排除 / Troubleshooting

### 问题：无法播放 MIDI 文件
**解决方案：**
- 确认已安装 WildMIDI 库
- 检查 Timidity 配置文件是否存在
- 验证 MIDI 文件格式正确

### 问题：MIDI 没有声音
**解决方案：**
- 安装音色库（freepats）
- 检查 `/etc/timidity/timidity.cfg` 配置
- 验证音量设置不为 0

### 问题：音色听起来不对
**解决方案：**
- 安装更好的 SoundFont
- 检查 Timidity 配置指向的音色库
- 尝试不同的 MIDI 文件

## 相关文档 / Related Documentation

- [AudioPlayer 分析文档](../../AudioPlayer_Analysis.md)
- [快速参考指南](../../QUICK_REFERENCE.md)
- [WildMIDI 插件 README](README.md)
