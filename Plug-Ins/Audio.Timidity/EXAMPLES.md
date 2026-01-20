# Libtimidity Plugin 使用示例 / Usage Examples

## 基本说明 / Basic Information

Libtimidity 插件提供与 WildMIDI 插件相同的接口，使用方法完全一致。两个插件都使用相同的插件名称 "MIDI"，因此只需安装其中一个即可。

The Libtimidity plugin provides the same interface as the WildMIDI plugin. Usage is identical. Both plugins use the same plugin name "MIDI", so you only need one installed.

## 选择哪个插件 / Which Plugin to Choose

### 使用 Libtimidity 的场景 / When to Use Libtimidity
- 需要轻量级的 MIDI 播放
- 系统上已有 libtimidity
- 简单的 MIDI 文件
- 较小的依赖占用

### 使用 WildMIDI 的场景 / When to Use WildMIDI
- 需要更好的音质
- 复杂的 MIDI 文件
- 更活跃的维护
- 更好的兼容性

## 基本用法 / Basic Usage

### 示例 1: 使用 AudioPlayer 播放 MIDI 文件 / Example 1: Play MIDI with AudioPlayer

```cpp
#include<hgl/audio/AudioPlayer.h>

// 创建 MIDI 播放器 - 无论使用哪个插件，代码都一样
// Create MIDI player - code is the same regardless of which plugin is used
AudioPlayer midi_player;

// 加载 MIDI 文件
// Load MIDI file
if (midi_player.Load("background_music.mid"))
{
    // 设置音量
    // Set volume
    midi_player.SetGain(0.8f);
    
    // 开始循环播放
    // Start loop playback
    midi_player.Play(true);
}
```

### 示例 2: 从内存加载 MIDI / Example 2: Load MIDI from Memory

```cpp
#include<hgl/audio/AudioPlayer.h>
#include<hgl/io/MemoryInputStream.h>

// MIDI 数据已在内存中
// MIDI data already in memory
unsigned char *midi_data = ...;
int midi_size = ...;

// 创建内存输入流
// Create memory input stream
io::MemoryInputStream stream(midi_data, midi_size);

// 从流中加载
// Load from stream
AudioPlayer player;
if (player.Load(&stream, midi_size, AudioFileType::MIDI))
{
    player.Play(true);
}
```

### 示例 3: 使用 AudioBuffer 播放短 MIDI / Example 3: Play Short MIDI with AudioBuffer

```cpp
#include<hgl/audio/AudioBuffer.h>
#include<hgl/audio/AudioSource.h>

// 加载 MIDI 音效
// Load MIDI sound effect
AudioBuffer midi_sfx("jingle.midi");

// 创建音源并播放
// Create audio source and play
AudioSource source;
if (source.Link(&midi_sfx))
{
    source.SetGain(1.0f);
    source.Play();
}
```

## 配置 / Configuration

### 环境变量 / Environment Variable

```bash
# 设置自定义 Timidity 配置路径
# Set custom Timidity configuration path
export TIMIDITY_CFG=/path/to/timidity.cfg
```

### 默认配置路径 / Default Configuration Paths

- Linux/Unix: `/etc/timidity/timidity.cfg`
- Windows: `C:\timidity\timidity.cfg`

## 安装和配置 / Installation and Configuration

### Ubuntu/Debian

```bash
# 安装 libtimidity 和 Timidity
sudo apt-get install libtimidity-dev timidity freepats

# 验证配置文件
ls -l /etc/timidity/timidity.cfg

# 测试 Timidity（可选）
timidity test.mid
```

### Fedora/RHEL

```bash
# 安装 libtimidity 和 Timidity++
sudo dnf install libtimidity-devel timidity++

# 配置音色库
sudo dnf install timidity-freepats
```

## 与 WildMIDI 插件共存 / Coexistence with WildMIDI

### 同时安装两个插件 / Installing Both Plugins

可以同时编译安装两个 MIDI 插件，但运行时只会使用其中一个：

Both MIDI plugins can be compiled and installed simultaneously, but only one will be used at runtime:

```bash
# 编译时都会构建（如果依赖库都已安装）
# Both will be built during compilation (if dependencies are installed)
mkdir build
cd build
cmake ..
make

# 运行时，插件加载器会使用第一个找到的
# At runtime, the plugin loader will use the first one found
```

### 选择使用哪个插件 / Choosing Which Plugin to Use

插件加载顺序由系统决定。如果需要特定插件，可以：

Plugin loading order is determined by the system. To use a specific plugin:

1. **只安装一个插件**
   - 只安装 libtimidity-dev (使用 Timidity 插件)
   - 只安装 libwildmidi-dev (使用 WildMIDI 插件)

2. **重命名插件文件**（高级用法）
   - 可以重命名或移除不需要的插件共享库

## 性能对比 / Performance Comparison

### 内存使用 / Memory Usage
- **Libtimidity**: 较低内存占用
- **WildMIDI**: 稍高内存占用

### 音质 / Sound Quality
- **Libtimidity**: 适合简单 MIDI
- **WildMIDI**: 复杂 MIDI 音质更好

### 初始化速度 / Initialization Speed
- **Libtimidity**: 更快
- **WildMIDI**: 稍慢

## 故障排除 / Troubleshooting

### 问题：插件初始化失败
**解决方案：**
```bash
# 检查配置文件是否存在
ls -l /etc/timidity/timidity.cfg

# 检查音色库是否安装
dpkg -l | grep freepats  # Debian/Ubuntu
rpm -qa | grep freepats  # Fedora/RHEL

# 手动测试 timidity
timidity --version
```

### 问题：音色不正确
**解决方案：**
```bash
# 重新安装音色库
sudo apt-get install --reinstall freepats

# 检查配置文件内容
cat /etc/timidity/timidity.cfg
```

### 问题：无法加载 MIDI 文件
**解决方案：**
- 验证 MIDI 文件格式正确
- 检查文件权限
- 确认 libtimidity 库已正确安装

## 高级示例 / Advanced Examples

### 示例 4: 动态切换背景音乐 / Example 4: Dynamic Background Music Switching

```cpp
#include<hgl/audio/AudioPlayer.h>

class MusicManager
{
    AudioPlayer player;
    
public:
    void PlayTrack(const os_char *filename, float fade_time = 2.0)
    {
        // 淡出当前音乐
        // Fade out current music
        if (player.GetPlayState() == PlayState::Play)
        {
            player.AutoGain(0.0f, fade_time, GetTimeSec());
            WaitTime(fade_time);
            player.Stop();
        }
        
        // 加载新音乐
        // Load new music
        if (player.Load(filename))
        {
            player.SetFadeTime(fade_time, fade_time);
            player.Play(true);
        }
    }
};

// 使用
// Usage
MusicManager music;
music.PlayTrack("menu.mid");
// ... later
music.PlayTrack("battle.mid");
```

## 相关文档 / Related Documentation

- [AudioPlayer 分析文档](../../AudioPlayer_Analysis.md)
- [快速参考指南](../../QUICK_REFERENCE.md)
- [Libtimidity 插件 README](README.md)
- [WildMIDI 插件文档](../Audio.WildMidi/README.md)

## 注意事项 / Notes

1. **插件名称**: 两个 MIDI 插件使用相同的插件名称 "MIDI"
2. **文件扩展名**: 都支持 `.mid` 和 `.midi`
3. **API 兼容**: 使用方法完全相同
4. **性能差异**: 根据使用场景选择合适的插件
5. **配置文件**: 都使用 Timidity 配置格式
