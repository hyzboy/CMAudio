# TinySoundFont Plugin 使用示例 / Usage Examples

## 基本说明 / Basic Information

TinySoundFont 插件的最大优势是**无需外部库依赖**，始终可以编译。使用方法与其他 MIDI 插件完全相同。

The biggest advantage of the TinySoundFont plugin is **no external library dependencies** - it always compiles. Usage is identical to other MIDI plugins.

## 四个 MIDI 插件对比 / Comparison of Four MIDI Plugins

### TinySoundFont (本插件 / This Plugin)
- ✅ 无依赖 - 头文件库，始终可编译
- ✅ 易于部署 - 无需安装外部库
- ✅ 跨平台 - 任何支持 C++ 的平台
- ✅ 良好音质 - 基于 SoundFont 合成
- ⚠️ 需要 SoundFont 文件

### FluidSynth
- ✅ 最佳音质
- ✅ 专业级功能
- ⚠️ 需要 libfluidsynth-dev
- ⚠️ 较高资源占用

### WildMIDI
- ✅ 良好音质
- ✅ 活跃维护
- ⚠️ 需要 libwildmidi-dev

### Libtimidity
- ✅ 最轻量
- ✅ 最快初始化
- ⚠️ 需要 libtimidity-dev

## 基本用法 / Basic Usage

### 示例 1: 使用 AudioPlayer 播放 MIDI 文件 / Example 1: Play MIDI with AudioPlayer

```cpp
#include<hgl/audio/AudioPlayer.h>

// 创建 MIDI 播放器 - 所有插件代码完全相同
// Create MIDI player - code is identical for all plugins
AudioPlayer midi_player;

// 加载 MIDI 文件
// Load MIDI file
if (midi_player.Load("game_music.mid"))
{
    // TinySoundFont 提供良好音质且无需外部库
    // TinySoundFont provides good quality without external libraries
    midi_player.SetGain(0.8f);
    midi_player.Play(true);
}
```

### 示例 2: 快速原型开发 / Example 2: Rapid Prototyping

```cpp
#include<hgl/audio/AudioPlayer.h>

// TinySoundFont 特别适合快速原型开发
// TinySoundFont is ideal for rapid prototyping
// 无需安装外部库，立即可用
// No need to install external libraries, works immediately

AudioPlayer bgm("background.mid");
bgm.Play(true);

// 就这么简单！只需确保有 SoundFont 文件
// That's it! Just ensure you have a SoundFont file
```

### 示例 3: 跨平台部署 / Example 3: Cross-Platform Deployment

```cpp
#include<hgl/audio/AudioPlayer.h>

class CrossPlatformMusicPlayer
{
    AudioPlayer player;
    
public:
    bool Initialize()
    {
        // TinySoundFont 在所有平台都能编译
        // TinySoundFont compiles on all platforms
        // 只需在运行时提供 SoundFont 文件
        // Just provide SoundFont file at runtime
        return true;
    }
    
    bool LoadAndPlay(const os_char* filename)
    {
        if (player.Load(filename))
        {
            player.Play(true);
            return true;
        }
        return false;
    }
};

// 使用
CrossPlatformMusicPlayer music;
music.Initialize();
music.LoadAndPlay("theme.mid");
```

### 示例 4: 配置 SoundFont 路径 / Example 4: Configure SoundFont Path

```bash
#!/bin/bash
# 设置 SoundFont 路径
# Set SoundFont path

# 使用默认 SoundFont
# Use default SoundFont
export TSF_SOUNDFONT=/usr/share/sounds/sf2/FluidR3_GM.sf2

# 或使用高质量 SoundFont
# Or use high-quality SoundFont
export TSF_SOUNDFONT=/path/to/GeneralUser_GS.sf2

# 运行程序
# Run program
./your_application
```

### 示例 5: 游戏音乐系统 / Example 5: Game Music System

```cpp
#include<hgl/audio/AudioPlayer.h>
#include<map>

class SimpleMusicManager
{
    std::map<std::string, AudioPlayer*> tracks;
    AudioPlayer* current;
    
public:
    SimpleMusicManager() : current(nullptr) {}
    
    ~SimpleMusicManager()
    {
        for (auto& pair : tracks)
            delete pair.second;
    }
    
    void AddTrack(const std::string& name, const os_char* file)
    {
        AudioPlayer* p = new AudioPlayer();
        if (p->Load(file))
            tracks[name] = p;
        else
            delete p;
    }
    
    void Play(const std::string& name, float fade = 1.0f)
    {
        if (current)
        {
            current->AutoGain(0.0f, fade, GetTimeSec());
            WaitTime(fade);
            current->Stop();
        }
        
        auto it = tracks.find(name);
        if (it != tracks.end())
        {
            current = it->second;
            current->SetFadeTime(fade, fade);
            current->Play(true);
        }
    }
};

// 使用示例
SimpleMusicManager music;
music.AddTrack("menu", "menu.mid");
music.AddTrack("level1", "level1.mid");
music.AddTrack("boss", "boss.mid");

music.Play("menu");
// ... 玩家开始游戏
music.Play("level1", 2.0f);
// ... 遇到 Boss
music.Play("boss", 1.0f);
```

## 安装和配置 / Installation and Configuration

### 编译时 / Build Time

```bash
# TinySoundFont 无需任何外部库
# TinySoundFont needs no external libraries
cd CMAudio
mkdir build
cd build
cmake ..
make

# 始终成功编译！
# Always compiles successfully!
```

### 运行时 / Runtime

```bash
# 只需安装 SoundFont 文件
# Only need to install SoundFont file
sudo apt-get install fluid-soundfont-gm

# 验证 SoundFont 位置
# Verify SoundFont location
ls -l /usr/share/sounds/sf2/FluidR3_GM.sf2
```

## 与其他插件对比 / Comparison with Other Plugins

### 编译难度 / Build Complexity

| 插件 | 编译依赖 | 是否总能编译 |
|------|---------|------------|
| TinySoundFont | 无 | ✅ 是 |
| FluidSynth | libfluidsynth-dev | ❌ 否 |
| WildMIDI | libwildmidi-dev | ❌ 否 |
| Libtimidity | libtimidity-dev | ❌ 否 |

### 部署难度 / Deployment Complexity

| 插件 | 运行时依赖 | 配置文件 |
|------|-----------|---------|
| TinySoundFont | SoundFont 文件 | 无 |
| FluidSynth | libfluidsynth + SoundFont | 无 |
| WildMIDI | libwildmidi | Timidity cfg |
| Libtimidity | libtimidity | Timidity cfg |

## 高级示例 / Advanced Examples

### 示例 6: 独立的 MIDI 播放器应用 / Example 6: Standalone MIDI Player Application

```cpp
#include<hgl/audio/AudioPlayer.h>
#include<iostream>

class SimpleMIDIPlayer
{
    AudioPlayer player;
    
public:
    bool Load(const char* filename)
    {
        std::cout << "Loading: " << filename << std::endl;
        return player.Load(filename);
    }
    
    void Play(bool loop = true)
    {
        std::cout << "Playing..." << std::endl;
        player.Play(loop);
    }
    
    void Stop()
    {
        std::cout << "Stopped" << std::endl;
        player.Stop();
    }
    
    double GetTime()
    {
        return player.GetPlayTime();
    }
    
    double GetTotalTime()
    {
        return player.GetTotalTime();
    }
};

// main 函数
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <midi_file>" << std::endl;
        return 1;
    }
    
    SimpleMIDIPlayer player;
    if (player.Load(argv[1]))
    {
        player.Play(false);
        
        // 等待播放完成
        while (player.GetTime() < player.GetTotalTime())
        {
            WaitTime(1.0);
            std::cout << "Time: " << player.GetTime() 
                     << " / " << player.GetTotalTime() << std::endl;
        }
    }
    
    return 0;
}
```

### 示例 7: 嵌入式系统应用 / Example 7: Embedded System Application

```cpp
// TinySoundFont 特别适合嵌入式系统
// TinySoundFont is ideal for embedded systems
// - 无外部依赖
// - 代码量小
// - 资源占用可控

#include<hgl/audio/AudioPlayer.h>

class EmbeddedMusicPlayer
{
    AudioPlayer player;
    bool initialized;
    
public:
    EmbeddedMusicPlayer() : initialized(false) {}
    
    bool Initialize(const char* soundfont_path)
    {
        // 设置 SoundFont 路径
        setenv("TSF_SOUNDFONT", soundfont_path, 1);
        initialized = true;
        return true;
    }
    
    bool PlayFile(const char* midi_file)
    {
        if (!initialized)
            return false;
            
        return player.Load(midi_file) && player.Play(true);
    }
};
```

## 故障排除 / Troubleshooting

### 问题：找不到 SoundFont 文件
**解决方案：**
```bash
# 安装默认 SoundFont
sudo apt-get install fluid-soundfont-gm

# 或手动设置路径
export TSF_SOUNDFONT=/path/to/your.sf2

# 验证文件存在
ls -l $TSF_SOUNDFONT
```

### 问题：音质不满意
**解决方案：**
```bash
# 使用更高质量的 SoundFont
# GeneralUser GS (推荐)
wget http://www.schristiancollins.com/generaluser/GeneralUser_GS_1.471.zip
unzip GeneralUser_GS_1.471.zip
export TSF_SOUNDFONT=/path/to/GeneralUser_GS.sf2
```

### 问题：编译错误
**解决方案：**
TinySoundFont 应该总是能编译。如果遇到问题：
- 确认 tsf.h 和 tml.h 在插件目录中
- 检查 C++ 编译器版本（需要 C++11 或更高）

### 问题：多个 MIDI 插件冲突
**说明：**
所有 MIDI 插件使用相同名称 "MIDI"，只有一个会被激活。

**优先使用 TinySoundFont：**
- 不安装其他 MIDI 库的开发包
- TinySoundFont 总是会被编译

## 最佳实践 / Best Practices

### 1. SoundFont 选择
- 开发：使用 FluidR3_GM（平衡质量和大小）
- 发布：提供高质量 SoundFont 或让用户选择

### 2. 跨平台部署
- 将 SoundFont 文件包含在应用程序包中
- 使用相对路径或配置文件指定位置

### 3. 性能优化
- 预加载常用 MIDI 文件
- 使用合适大小的 SoundFont（不是越大越好）

## 相关文档 / Related Documentation

- [AudioPlayer 分析文档](../../AudioPlayer_Analysis.md)
- [快速参考指南](../../QUICK_REFERENCE.md)
- [TinySoundFont 插件 README](README.md)
- [FluidSynth 插件文档](../Audio.FluidSynth/README.md)
- [WildMIDI 插件文档](../Audio.WildMidi/README.md)
- [Libtimidity 插件文档](../Audio.Timidity/README.md)

## 总结 / Summary

TinySoundFont 插件的核心优势：
- ✅ **零依赖**: 无需外部库，始终可编译
- ✅ **易部署**: 只需 SoundFont 文件
- ✅ **跨平台**: 所有平台都支持
- ✅ **良好音质**: SoundFont 合成
- ✅ **开发友好**: 快速原型，简单集成

The core advantages of TinySoundFont plugin:
- ✅ **Zero Dependencies**: No external libraries, always compiles
- ✅ **Easy Deployment**: Only needs SoundFont file
- ✅ **Cross-Platform**: Works on all platforms
- ✅ **Good Quality**: SoundFont-based synthesis
- ✅ **Developer Friendly**: Rapid prototyping, simple integration
