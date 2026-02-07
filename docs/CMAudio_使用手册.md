# CMAudio 使用手册

## 目录

1. [项目概述](#项目概述)
2. [系统架构](#系统架构)
3. [快速开始](#快速开始)
4. [核心模块](#核心模块)
5. [高级功能](#高级功能)
6. [插件系统](#插件系统)
7. [示例程序](#示例程序)
8. [API 参考](#api-参考)
9. [常见问题](#常见问题)
10. [附录](#附录)

---

## 项目概述

### 简介

CMAudio 是一个基于 OpenAL 的跨平台音频处理库，提供完整的音频播放、3D 空间音频、音频混音、MIDI 播放等功能。该库设计用于游戏开发、音频应用和多媒体项目。

### 主要特性

- **多格式支持**：WAV、OGG Vorbis、Opus、MIDI 等多种音频格式
- **3D 空间音频**：完整的空间音频系统，支持位置、速度、方向、距离衰减等
- **音频混音**：离线多轨混音，支持时间偏移、音量调整、音调变化
- **MIDI 播放**：专业级 MIDI 播放器，支持多种音色库（FluidSynth、WildMIDI、Timidity）
- **音频特效**：EFX 滤波器（低通、高通、带通）、混响、多普勒效果
- **插件架构**：可扩展的插件系统，支持自定义音频格式
- **高性能**：内存池管理、多线程播放、优化的混音算法

### 应用场景

- 游戏音频系统
- 音乐播放器
- 音频编辑工具
- 虚拟现实/增强现实音频
- 音乐教学软件
- 声音设计工具

---

## 系统架构

### 架构图

```
┌─────────────────────────────────────────────────────────┐
│                    应用层                                │
│   (游戏、音乐播放器、VR应用等)                             │
└─────────────────────────────────────────────────────────┘
                          │
┌─────────────────────────────────────────────────────────┐
│                  CMAudio 核心层                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ AudioManager │  │ AudioPlayer  │  │ MIDIPlayer   │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ AudioSource  │  │ AudioBuffer  │  │ AudioMixer   │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │SpatialAudio  │  │AudioResampler│  │MIDI          │  │
│  │   World      │  └──────────────┘  │Orchestra     │  │
│  └──────────────┘                    │Player        │  │
│                                       └──────────────┘  │
└─────────────────────────────────────────────────────────┘
                          │
┌─────────────────────────────────────────────────────────┐
│                  插件层                                  │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐  │
│  │Audio.WAV │ │Audio.Ogg │ │Audio.Opus│ │Audio.MIDI│  │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘  │
│  (FluidSynth / WildMIDI / Timidity / ADLMIDI / etc)    │
└─────────────────────────────────────────────────────────┘
                          │
┌─────────────────────────────────────────────────────────┐
│                   底层依赖                               │
│         OpenAL / libsamplerate / 第三方库                │
└─────────────────────────────────────────────────────────┘
```

### 模块职责

| 模块 | 职责 |
|------|------|
| **AudioManager** | 简单音效管理，适合快速播放短音效 |
| **AudioBuffer** | 音频数据缓冲区管理，存储解码后的音频数据 |
| **AudioSource** | 单个发声源控制，管理播放状态和空间属性 |
| **AudioPlayer** | 音频文件播放器，适合背景音乐等长音频 |
| **MIDIPlayer** | MIDI 文件播放器，支持实时通道控制 |
| **MIDIOrchestraPlayer** | 3D 空间交响乐团播放器，每个通道独立 3D 位置 |
| **AudioMixer** | 离线音频混音器，多轨混音与导出 |
| **SpatialAudioWorld** | 3D 空间音频场景管理 |
| **AudioResampler** | 音频重采样，格式转换 |

---

## 快速开始

### 环境要求

- **编译器**：支持 C++17 的编译器（GCC 7+, Clang 5+, MSVC 2019+）
- **CMake**：3.28 或更高版本
- **依赖库**：OpenAL、libsamplerate

### 安装依赖

#### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install libopenal-dev libsamplerate0-dev
# 可选：MIDI 支持
sudo apt-get install libfluidsynth-dev fluid-soundfont-gm
```

#### Fedora/CentOS

```bash
sudo dnf install openal-soft-devel libsamplerate-devel
# 可选：MIDI 支持
sudo dnf install fluidsynth-devel fluid-soundfont-gm
```

#### macOS

```bash
brew install openal-soft libsamplerate
# 可选：MIDI 支持
brew install fluidsynth
```

#### Windows

使用 vcpkg 安装依赖：

```bash
vcpkg install openal-soft libsamplerate
# 可选：MIDI 支持
vcpkg install fluidsynth
```

### 编译构建

```bash
# 克隆仓库
git clone https://github.com/hyzboy/CMAudio.git
cd CMAudio

# 初始化子模块
git submodule update --init --recursive

# 创建构建目录
mkdir build
cd build

# 配置
cmake ..

# 编译
make -j$(nproc)

# 可选：禁用示例程序
cmake .. -DBUILD_EXAMPLES=OFF
make -j$(nproc)
```

### 第一个程序

#### 简单音效播放

```cpp
#include <hgl/audio/AudioManager.h>

using namespace hgl;

int main()
{
    // 创建音频管理器（8个音源）
    AudioManager manager(8);
    
    // 播放音效
    manager.Play(OS_TEXT("sound.wav"), 1.0f);  // 音量 1.0
    
    // 等待播放完成
    sleep(2);
    
    return 0;
}
```

#### 背景音乐播放

```cpp
#include <hgl/audio/AudioPlayer.h>

using namespace hgl;

int main()
{
    // 创建播放器
    AudioPlayer player;
    
    // 加载音频文件
    if(player.Load(OS_TEXT("music.ogg")))
    {
        player.SetLoop(true);   // 循环播放
        player.SetGain(0.8f);   // 音量 80%
        player.Play();          // 开始播放
    }
    
    // 等待用户操作...
    getchar();
    
    player.Stop();
    
    return 0;
}
```

#### 3D 空间音频

```cpp
#include <hgl/audio/AudioBuffer.h>
#include <hgl/audio/AudioSource.h>
#include <hgl/math/Vector.h>

using namespace hgl;
using namespace hgl::math;

int main()
{
    // 加载音频缓冲
    AudioBuffer buffer(OS_TEXT("fire.wav"));
    
    // 创建音源
    AudioSource source(&buffer);
    
    // 设置 3D 位置
    source.SetPosition(Vector3f(10.0f, 0.0f, 0.0f));
    
    // 设置距离衰减
    source.SetDistance(1.0f, 50.0f);  // 参考距离 1m，最大距离 50m
    
    // 播放
    source.Play(true);  // 循环播放
    
    // 模拟移动
    for(int i = 0; i < 100; i++)
    {
        float x = 10.0f * cosf(i * 0.1f);
        float z = 10.0f * sinf(i * 0.1f);
        source.SetPosition(Vector3f(x, 0.0f, z));
        usleep(50000);  // 50ms
    }
    
    return 0;
}
```

---

## 核心模块

### AudioManager - 简单音效管理

`AudioManager` 提供最简单的音效播放功能，适合游戏中的短音效。

#### 特点

- 自动管理音源池
- 简单的 API
- 适合快速原型开发

#### 基本用法

```cpp
#include <hgl/audio/AudioManager.h>

using namespace hgl;

// 创建管理器（默认 8 个音源）
AudioManager manager(8);

// 播放音效
manager.Play(OS_TEXT("explosion.wav"), 1.0f);    // 爆炸声
manager.Play(OS_TEXT("coin.wav"), 0.5f);         // 拾取声，音量 50%
```

#### 注意事项

- 音源数量有限，同时播放不能超过创建时指定的数量
- 音效文件会被缓存，重复播放会共享缓冲区
- 不支持精细控制，如需高级功能请使用 `AudioSource`

### AudioBuffer - 音频缓冲区

`AudioBuffer` 管理解码后的音频数据，所有音频播放都需要通过缓冲区。

#### 支持的格式

- **WAV**：标准 PCM WAV 文件
- **OGG Vorbis**：开源音频压缩格式
- **Opus**：高质量音频编解码
- **MIDI**：通过插件支持

#### 加载音频

```cpp
#include <hgl/audio/AudioBuffer.h>

using namespace hgl;

// 从文件加载
AudioBuffer buffer1(OS_TEXT("sound.wav"));

// 指定格式加载
AudioBuffer buffer2(OS_TEXT("music.ogg"), AudioFileType::OGG);

// 从内存加载
void* data = ...;  // 音频数据
int size = ...;    // 数据大小
AudioBuffer buffer3(data, size, AudioFileType::WAV);

// 查询信息
double duration = buffer1.GetTime();      // 时长（秒）
uint sampleRate = buffer1.GetFreq();      // 采样率
uint dataSize = buffer1.GetSize();        // 数据大小
```

#### 格式自动检测

```cpp
// 不指定格式，自动根据扩展名检测
AudioBuffer buffer(OS_TEXT("audio.ogg"));  // 自动识别为 OGG
```

### AudioSource - 音频源

`AudioSource` 是单个发声源的控制对象，提供完整的播放控制和空间音频功能。

#### 播放控制

```cpp
#include <hgl/audio/AudioSource.h>
#include <hgl/audio/AudioBuffer.h>

using namespace hgl;

AudioBuffer buffer(OS_TEXT("sound.wav"));
AudioSource source;

// 绑定缓冲区
source.Link(&buffer);

// 播放控制
source.Play();              // 播放一次
source.Play(true);          // 循环播放
source.Pause();             // 暂停
source.Resume();            // 继续
source.Stop();              // 停止
source.Rewind();            // 重绕到开头

// 查询状态
bool isPlaying = source.IsPlaying();
bool isPaused = source.IsPaused();
bool isStopped = source.IsStopped();
```

#### 音量和音调

```cpp
// 音量控制（0.0 - 1.0+）
source.SetGain(0.8f);           // 80% 音量
float volume = source.GetGain();

// 音调控制（0.5 - 2.0）
source.SetPitch(1.2f);          // 提高 20% 音调
source.SetPitch(0.8f);          // 降低 20% 音调
```

#### 3D 空间属性

```cpp
#include <hgl/math/Vector.h>

using namespace hgl::math;

// 位置
source.SetPosition(Vector3f(10.0f, 2.0f, 5.0f));
Vector3f pos = source.GetPosition();

// 速度（用于多普勒效果）
source.SetVelocity(Vector3f(5.0f, 0.0f, 0.0f));

// 方向（用于定向声源）
source.SetDirection(Vector3f(1.0f, 0.0f, 0.0f));
```

#### 距离衰减

```cpp
// 设置距离范围
source.SetDistance(
    1.0f,      // 参考距离：在此距离内音量不衰减
    100.0f     // 最大距离：超过此距离音量为 0
);

// 衰减系数（默认 1.0）
source.SetRolloffFactor(2.0f);  // 加快衰减
source.SetRolloffFactor(0.5f);  // 减缓衰减

// 距离模型
source.SetDistanceModel(AL_LINEAR_DISTANCE);
source.SetDistanceModel(AL_INVERSE_DISTANCE);
source.SetDistanceModel(AL_EXPONENT_DISTANCE);
```

#### 锥形音源

模拟定向声源（如扬声器、喇叭）：

```cpp
#include <hgl/audio/ConeAngle.h>

ConeAngle cone;
cone.inner = 30.0f;   // 内锥角度（全音量）
cone.outer = 60.0f;   // 外锥角度（衰减音量）

source.SetConeAngle(cone);
source.SetConeGain(0.3f);  // 外锥外的音量衰减到 30%
```

#### EFX 滤波器

需要硬件支持 OpenAL EFX 扩展：

```cpp
// 低通滤波器（过滤高频）
source.SetLowpassFilter(
    1.0f,    // 总增益
    0.5f     // 高频增益（0.0 完全过滤，1.0 不过滤）
);

// 高通滤波器（过滤低频）
source.SetHighpassFilter(
    1.0f,    // 总增益
    0.5f     // 低频增益
);

// 带通滤波器（同时过滤高低频）
source.SetBandpassFilter(
    1.0f,    // 总增益
    0.3f,    // 低频增益
    0.7f     // 高频增益
);

// 禁用滤波器
source.DisableFilter();
```

#### 多普勒效果

```cpp
// 设置多普勒强度（0.0 - 10.0，默认 0.0）
source.SetDopplerFactor(1.0f);

// 设置多普勒速度
source.SetDopplerVelocity(343.3f);  // 声速 m/s
```

#### 空气吸收

模拟空气对声音的吸收（距离越远高频损失越多）：

```cpp
// 空气吸收因子（0.0 - 10.0，默认 0.0）
source.SetAirAbsorptionFactor(0.5f);
```

### AudioPlayer - 音频播放器

`AudioPlayer` 是面向长音频文件的播放器，内部使用独立线程，适合背景音乐。

#### 基本用法

```cpp
#include <hgl/audio/AudioPlayer.h>

using namespace hgl;

AudioPlayer player;

// 加载音频
if(player.Load(OS_TEXT("bgm.ogg")))
{
    // 设置循环
    player.SetLoop(true);
    
    // 设置音量
    player.SetGain(0.7f);
    
    // 播放
    player.Play();
}

// 播放控制
player.Pause();     // 暂停
player.Resume();    // 继续
player.Stop();      // 停止
```

#### 淡入淡出

```cpp
#include <hgl/time/Time.h>

using namespace hgl;

// 设置淡入淡出时间
player.SetFadeTime(
    2.0,    // 淡入时间（秒）
    3.0     // 淡出时间（秒）
);

player.Play();
```

#### 自动音量调节

```cpp
// 在指定时间内将音量调整到目标值
double currentTime = GetCurrentTime();
player.AutoGain(
    0.5f,         // 目标音量
    5.0,          // 持续时间（秒）
    currentTime   // 当前时间
);
```

#### 查询播放状态

```cpp
// 获取播放时间
PreciseTime playTime = player.GetPlayTime();

// 获取总时长
double totalTime = player.GetTotalTime();

// 获取播放状态
PlayState state = player.GetPlayState();
// PlayState::None - 未播放
// PlayState::Play - 正在播放
// PlayState::Pause - 暂停
// PlayState::Exit - 已退出
```

#### 3D 空间属性

`AudioPlayer` 也支持 3D 空间音频：

```cpp
// 位置
player.SetPosition(Vector3f(0.0f, 5.0f, 10.0f));

// 速度
player.SetVelocity(Vector3f(1.0f, 0.0f, 0.0f));

// 距离
player.SetDistance(1.0f, 50.0f);
```

### AudioMixer - 音频混音器

`AudioMixer` 提供离线多轨混音功能，适合音频制作和批处理。

#### 特点

- **浮点混音**：内部使用 float 格式（-1.0 到 1.0），避免削波失真
- **无损质量**：多轨叠加不会产生累积误差
- **软削波**：可选的 tanh 软削波算法，平滑处理超限信号
- **抖动**：可选的 TPDF 抖动，减少量化噪声
- **灵活输出**：支持 int16 或 float32 输出

#### 基本流程

```cpp
#include <hgl/audio/AudioMixer.h>

using namespace hgl::audio;

// 1. 创建混音器
AudioMixer mixer;

// 2. 添加音源
int source1 = mixer.AddSourceAudio(data1, size1, AL_FORMAT_MONO16, 44100);
int source2 = mixer.AddSourceAudio(data2, size2, AL_FORMAT_MONO16, 44100);

// 3. 添加混音轨道
mixer.AddTrack(
    source1,    // 音源索引
    0.0f,       // 时间偏移（秒）
    1.0f,       // 音量（0.0 - 1.0）
    1.0f        // 音调（0.5 - 2.0）
);

mixer.AddTrack(source1, 0.5f, 0.7f, 0.95f);   // 延迟 0.5秒，音量 70%
mixer.AddTrack(source2, 1.0f, 0.8f, 1.05f);   // 延迟 1.0秒，音调提高 5%

// 4. 配置混音器（可选）
MixerConfig config;
config.useSoftClipper = true;   // 启用软削波
config.useDither = true;        // 启用抖动
mixer.SetConfig(config);

// 5. 执行混音
void* outputData;
uint outputSize;
if(mixer.Mix(&outputData, &outputSize, 10.0f))  // 混音 10 秒
{
    // 使用输出数据...
    // outputData 需要手动释放
    delete[] (char*)outputData;
}
```

#### 高级配置

```cpp
// 混音器配置
MixerConfig config;

// 软削波：平滑处理超限信号
config.useSoftClipper = true;

// 抖动：减少量化噪声
config.useDither = true;

mixer.SetConfig(config);

// 设置输出格式
mixer.SetOutputFormat(AL_FORMAT_MONO16);       // 16位整数
mixer.SetOutputFormat(AL_FORMAT_MONO_FLOAT32); // 32位浮点
```

#### 应用场景

- 多轨音效叠加
- 音频剪辑拼接
- 批量音频处理
- 音效预渲染

### MIDIPlayer - MIDI 播放器

`MIDIPlayer` 提供专业级 MIDI 文件播放，支持实时通道控制和多种音色库。

#### 支持的 MIDI 后端

| 后端 | 音质 | CPU | 内存 | 配置文件 |
|------|------|-----|------|----------|
| **FluidSynth** | 优秀 | 高 | 高 | SoundFont (.sf2) |
| **WildMIDI** | 良好 | 中 | 中 | Timidity 配置 |
| **Timidity** | 良好 | 低 | 低 | Timidity 配置 |
| **ADLMIDI** | 良好 | 低 | 低 | OPL3 |
| **OPNMIDI** | 良好 | 低 | 低 | OPN2 |
| **TinySoundFont** | 良好 | 中 | 中 | SoundFont (.sf2) |

#### 基本播放

```cpp
#include <hgl/audio/MIDIPlayer.h>

using namespace hgl;

MIDIPlayer player;

// 设置音色库（FluidSynth）
player.SetSoundFont("/usr/share/sounds/sf2/FluidR3_GM.sf2");

// 设置采样率
player.SetSampleRate(44100);

// 加载 MIDI 文件
if(player.Load(OS_TEXT("music.mid")))
{
    player.SetLoop(true);
    player.Play();
}
```

#### 实时通道控制

```cpp
// 设置通道乐器（Program Change）
player.SetChannelProgram(
    0,      // 通道号（0-15）
    24      // 乐器编号（GM 标准）
);

// 设置通道音量
player.SetChannelVolume(0, 100);   // 音量 0-127

// 设置通道声像
player.SetChannelPan(0, 64);       // 声像 0-127（64=中央）

// 静音/独奏
player.SetChannelMute(1, true);    // 静音通道 1
player.SetChannelSolo(0, true);    // 独奏通道 0
```

#### 查询通道信息

```cpp
#include <string>

// 获取通道信息
MidiChannelInfo info;
if(player.GetChannelInfo(0, &info))
{
    std::cout << "通道 0:" << std::endl;
    std::cout << "  乐器: " << info.program << std::endl;
    std::cout << "  音量: " << info.volume << std::endl;
    std::cout << "  声像: " << info.pan << std::endl;
}

// 获取通道活跃状态
bool isActive = player.GetChannelActivity(0);
```

#### 多通道解码

离线解码每个通道到独立音频：

```cpp
// 解码通道 0
void* channelData;
uint channelSize;
if(player.DecodeChannel(0, &channelData, &channelSize))
{
    // 使用通道数据...
    delete[] (char*)channelData;
}

// 解码所有通道
for(int ch = 0; ch < 16; ch++)
{
    void* data;
    uint size;
    if(player.DecodeChannel(ch, &data, &size))
    {
        // 处理通道数据...
        delete[] (char*)data;
    }
}
```

### MIDIOrchestraPlayer - 3D 空间交响乐团播放器

`MIDIOrchestraPlayer` 是专为沉浸式 3D 空间音乐体验设计的高级 MIDI 播放器。它将每个 MIDI 通道映射为独立的 `AudioSource` 和 3D 位置，模拟真实乐队的空间布局。

#### 特点

- **16 个独立音源**：每个 MIDI 通道有独立的 `AudioSource` 和 3D 位置
- **预设乐队布局**：交响乐、室内乐、爵士、摇滚等布局
- **实时位置调整**：动态修改每个乐器的 3D 位置
- **完美同步**：所有通道完美同步播放
- **独立空间效果**：每个通道独立的距离衰减和空间音频

#### 与 MIDIPlayer 的对比

| 特性 | MIDIPlayer | MIDIOrchestraPlayer |
|------|-----------|-------------------|
| **音频输出** | 单个混合音源 | 16 个独立音源 |
| **3D 空间** | 整体位置 | 每个通道独立位置 |
| **适用场景** | 背景音乐 | VR 音乐厅、沉浸式体验 |
| **CPU 占用** | 低 | 高 |
| **内存占用** | 低 | 高 |
| **推荐后端** | 任意 MIDI 插件 | FluidSynth |

#### 预设布局

```cpp
enum class OrchestraLayout
{
    Standard,   // 标准交响乐团布局
    Chamber,    // 室内乐布局
    Jazz,       // 爵士乐布局
    Rock,       // 摇滚乐队布局
    Custom      // 自定义布局
};
```

**标准交响乐团布局 (Standard)**：
- 通道 0-3：弦乐组（前方，左右分布）
- 通道 4-7：木管组（中前方）
- 通道 8-11：铜管组（后方，宽广分布）
- 通道 12-15：打击乐和其他（侧方和后方）

**室内乐布局 (Chamber)**：
- 较小的空间范围
- 乐器更靠近听者
- 适合小型合奏

**爵士布局 (Jazz)**：
- 钢琴、贝司在中央
- 管乐在前方两侧
- 鼓组在后方中央

**摇滚布局 (Rock)**：
- 主唱在前方中央
- 吉他和贝司在两侧
- 鼓组在后方

#### 基本使用

```cpp
#include <hgl/audio/MIDIOrchestraPlayer.h>

using namespace hgl;

// 创建播放器
MIDIOrchestraPlayer orchestra;

// 设置音色库（推荐 FluidSynth）
orchestra.SetSoundFont("/usr/share/sounds/sf2/FluidR3_GM.sf2");
orchestra.SetSampleRate(44100);

// 加载 MIDI 文件
if(orchestra.Load(OS_TEXT("symphony.mid")))
{
    // 设置标准交响乐团布局
    orchestra.SetLayout(OrchestraLayout::Standard);
    
    // 设置乐团中心位置（舞台位置）
    orchestra.SetOrchestraCenter(Vector3f(0.0f, 0.0f, 10.0f));
    
    // 设置缩放（调整乐团大小）
    orchestra.SetOrchestraScale(1.5f);  // 1.5 倍大小
    
    // 开始播放
    orchestra.Play();
}
```

#### 自定义通道位置

```cpp
// 自定义布局
orchestra.SetLayout(OrchestraLayout::Custom);

// 设置各通道位置
orchestra.SetChannelPosition(0, Vector3f(-2.0f, 0.0f, 8.0f));  // 第一小提琴（左前）
orchestra.SetChannelPosition(1, Vector3f(2.0f, 0.0f, 8.0f));   // 第二小提琴（右前）
orchestra.SetChannelPosition(2, Vector3f(-1.5f, 0.0f, 9.0f));  // 中提琴（左中）
orchestra.SetChannelPosition(3, Vector3f(1.5f, 0.0f, 9.0f));   // 大提琴（右中）

// 设置通道音量
orchestra.SetChannelVolume(0, 0.9f);  // 第一小提琴稍弱
orchestra.SetChannelVolume(1, 1.0f);  // 第二小提琴正常

// 启用/禁用通道
orchestra.SetChannelEnabled(9, false);  // 禁用通道 9（通常是鼓）
```

#### 访问通道 AudioSource

可以直接访问每个通道的 `AudioSource` 以应用更多效果：

```cpp
// 获取通道 0 的 AudioSource
AudioSource* violin = orchestra.GetChannelSource(0);

if(violin)
{
    // 设置距离衰减
    violin->SetDistance(1.0f, 30.0f);
    
    // 应用低通滤波器（模拟远距离）
    violin->SetLowpassFilter(1.0f, 0.7f);
    
    // 设置 Rolloff 因子
    violin->SetRolloffFactor(1.5f);
}
```

#### 通道控制

```cpp
// 设置通道乐器
orchestra.SetChannelProgram(0, 40);  // 通道 0 设为小提琴

// 静音某个通道
orchestra.MuteChannel(9, true);  // 静音鼓通道

// 独奏某个通道（其他通道静音）
orchestra.SoloChannel(0, true);  // 只听小提琴
```

#### 淡入淡出

```cpp
// 淡入（5 秒）
orchestra.FadeIn(5.0f);

// 淡出（3 秒）
orchestra.FadeOut(3.0f);

// 自定义增益控制
orchestra.AutoGain(0.5f, 2.0f, 1.0f);  // 从 50% 增益过渡到 100%，持续 2 秒
```

#### 查询状态

```cpp
// 获取通道数量
int channelCount = orchestra.GetChannelCount();

// 获取通道信息
MidiChannelInfo info = orchestra.GetChannelInfo(0);
std::cout << "通道 0 乐器: " << info.program << std::endl;

// 获取通道位置
Vector3f pos = orchestra.GetChannelPosition(0);
std::cout << "位置: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;

// 检查播放状态
if(orchestra.IsPlaying())
{
    std::cout << "正在播放" << std::endl;
}
```

#### 应用场景

- **VR 音乐厅**：玩家可以在虚拟音乐厅中自由走动，聆听不同位置的乐器
- **游戏音乐**：在游戏中展示乐队表演，玩家可以听到真实的空间音效
- **音乐教育**：让学生了解乐团中每个乐器的位置和声音
- **沉浸式音频**：为电影、展览创建立体声音乐体验

#### VR 音乐厅示例

```cpp
#include <hgl/audio/MIDIOrchestraPlayer.h>
#include <hgl/audio/Listener.h>

using namespace hgl;

// 创建交响乐团
MIDIOrchestraPlayer orchestra;
orchestra.SetSoundFont("/path/to/soundfont.sf2");
orchestra.SetSampleRate(44100);

if(orchestra.Load(OS_TEXT("beethoven_symphony.mid")))
{
    // 设置标准布局
    orchestra.SetLayout(OrchestraLayout::Standard);
    
    // 舞台在前方 10 米
    orchestra.SetOrchestraCenter(Vector3f(0.0f, 0.0f, 10.0f));
    orchestra.SetOrchestraScale(2.0f);  // 大型音乐厅
    
    orchestra.Play(true);  // 循环播放
}

// 游戏主循环
while(running)
{
    // 更新听者（玩家/摄像机）位置
    Vector3f playerPos = GetPlayerPosition();
    Vector3f playerForward = GetPlayerForward();
    Vector3f playerUp = GetPlayerUp();
    
    Listener::SetPosition(playerPos);
    Listener::SetOrientation(playerForward, playerUp);
    
    // 玩家可以自由移动，听到不同位置的乐器
    // 靠近弦乐组时弦乐更响，靠近铜管时铜管更响
}
```

#### 性能考虑

**CPU 占用**：
- 16 个独立音源意味着更高的 CPU 占用
- 建议在高性能设备或 VR 应用中使用
- 可以通过禁用不需要的通道来降低 CPU 占用

**内存占用**：
- 每个通道需要独立的音频缓冲区
- 建议使用质量适中的 SoundFont（不要太大）

**优化建议**：

```cpp
// 禁用不活跃的通道
for(int ch = 0; ch < 16; ch++)
{
    if(!orchestra.GetChannelActivity(ch))
    {
        orchestra.SetChannelEnabled(ch, false);
    }
}

// 降低采样率（如果音质要求不高）
orchestra.SetSampleRate(22050);  // 降低一半采样率

// 使用较小的缩放比例（降低空间范围）
orchestra.SetOrchestraScale(0.5f);
```

#### 与其他系统集成

**与 SpatialAudioWorld 配合**：

```cpp
// 可以将乐团作为场景中的一个音源群
SpatialAudioWorld world;

// 每个通道的 AudioSource 可以注册到空间音频世界
for(int ch = 0; ch < 16; ch++)
{
    AudioSource* source = orchestra.GetChannelSource(ch);
    if(source)
    {
        // 应用全局空间音频设置
        source->SetAirAbsorptionFactor(0.1f);
        source->SetDopplerFactor(1.0f);
    }
}
```

#### 注意事项

- **依赖 FluidSynth**：推荐使用 FluidSynth 插件以获得最佳的多通道分离支持
- **系统资源**：16 个独立音源需要更多系统资源，不适合低端设备
- **同步性**：所有通道完美同步，但需要确保音频线程优先级足够高
- **音色库**：使用高质量 SoundFont 可获得更好的音质，但会占用更多内存

---

## 高级功能

### SpatialAudioWorld - 3D 空间音频世界

`SpatialAudioWorld` 提供完整的 3D 空间音频场景管理，包括音源管理、方向性、混响等高级功能。

#### 创建空间音频世界

```cpp
#include <hgl/audio/SpatialAudioWorld.h>

using namespace hgl;

// 创建空间音频世界
SpatialAudioWorld world;

// 配置音源
SpatialAudioSourceConfig config;
config.buffer = &audioBuffer;
config.position = Vector3f(10.0f, 0.0f, 0.0f);
config.gain = 1.0f;
config.rolloff_factor = 1.0f;
config.ref_distance = 1.0f;
config.max_distance = 100.0f;
config.loop = true;

// 添加音源到世界
auto source = world.CreateSource(config);

// 更新音源位置
source->SetPosition(Vector3f(5.0f, 2.0f, 3.0f));

// 每帧更新
world.Update(deltaTime);
```

#### 方向性增益

模拟声源的指向性（如喇叭、扬声器）：

```cpp
#include <hgl/audio/DirectionalGainPattern.h>

using namespace hgl::audio;

// 创建方向性图案
DirectionalGainPattern pattern;
pattern.SetCardioid();      // 心形指向
// 或
pattern.SetSupercardioid(); // 超心形
pattern.SetHypercardioid(); // 超指向
pattern.SetBidirectional(); // 双向
pattern.SetOmnidirectional(); // 全向

// 应用到音源
source->directional_pattern = pattern;
```

#### 混响预设

添加环境混响效果：

```cpp
#include <hgl/audio/ReverbPreset.h>

using namespace hgl;

// 使用预设混响
ReverbPreset::ApplyPreset(ReverbPreset::Hall);      // 大厅
ReverbPreset::ApplyPreset(ReverbPreset::Room);      // 房间
ReverbPreset::ApplyPreset(ReverbPreset::Cave);      // 洞穴
ReverbPreset::ApplyPreset(ReverbPreset::Arena);     // 竞技场
ReverbPreset::ApplyPreset(ReverbPreset::Hangar);    // 机库
```

### AudioResampler - 音频重采样

音频采样率转换和格式转换：

```cpp
#include <hgl/audio/AudioResampler.h>

using namespace hgl;

// 创建重采样器
AudioResampler resampler;

// 设置源格式
resampler.SetSourceFormat(44100, 1);  // 44.1kHz, 单声道

// 设置目标格式
resampler.SetTargetFormat(48000, 2);  // 48kHz, 立体声

// 重采样
float* input = ...;   // 输入数据
int inputFrames = ...;
float* output;
int outputFrames;

if(resampler.Process(input, inputFrames, &output, &outputFrames))
{
    // 使用输出数据...
    delete[] output;
}
```

### Listener - 听者（摄像机）

设置听者的位置和朝向：

```cpp
#include <hgl/audio/Listener.h>

using namespace hgl;

// 设置听者位置
Listener::SetPosition(Vector3f(0.0f, 0.0f, 0.0f));

// 设置听者朝向
Listener::SetOrientation(
    Vector3f(0.0f, 0.0f, -1.0f),  // 前方向
    Vector3f(0.0f, 1.0f, 0.0f)    // 上方向
);

// 设置听者速度（多普勒效果）
Listener::SetVelocity(Vector3f(0.0f, 0.0f, 0.0f));

// 设置主音量
Listener::SetGain(1.0f);
```

### AudioMemoryPool - 内存池

高效的音频缓冲区内存管理：

```cpp
#include <hgl/audio/AudioMemoryPool.h>

using namespace hgl::audio;

// 创建内存池
AudioMemoryPool<float> pool;

// 分配缓冲区
float* buffer = pool.Allocate(1024);  // 1024 个 float

// 使用缓冲区...

// 释放回池（不会真正删除，可复用）
pool.Free(buffer);
```

---

## 插件系统

### 音频格式插件

CMAudio 使用插件系统支持多种音频格式。每个格式都是一个独立的插件。

#### 已支持的格式

| 插件 | 格式 | 说明 |
|------|------|------|
| **Audio.WAV** | WAV | 标准 PCM 波形文件 |
| **Audio.Vorbis** | OGG | Ogg Vorbis 压缩格式 |
| **Audio.Tremor** | OGG | Vorbis 整数解码器（低 CPU） |
| **Audio.Tremolo** | OGG | Vorbis 定点解码器 |
| **Audio.Opus** | OPUS | 高质量音频编解码 |
| **Audio.FluidSynth** | MIDI | FluidSynth MIDI 合成器 |
| **Audio.WildMIDI** | MIDI | WildMIDI MIDI 播放器 |
| **Audio.Timidity** | MIDI | Timidity MIDI 播放器 |
| **Audio.ADLMIDI** | MIDI | AdLib/OPL3 MIDI 合成器 |
| **Audio.OPNMIDI** | MIDI | Yamaha OPN2 MIDI 合成器 |
| **Audio.TinySoundFont** | MIDI | 轻量级 SoundFont 合成器 |

#### 插件自动加载

插件在运行时自动加载，无需手动注册。音频文件格式由文件扩展名自动识别。

#### 选择 MIDI 插件

多个 MIDI 插件可同时安装，但只有一个会被激活。选择方法：

1. **只安装需要的库**

```bash
# 只安装 FluidSynth
sudo apt-get install libfluidsynth-dev
```

2. **设置音色库路径**

```bash
# FluidSynth
export FLUIDSYNTH_SF2=/path/to/soundfont.sf2

# WildMIDI / Timidity
export TIMIDITY_CFG=/path/to/timidity.cfg
```

#### 插件性能比较

**MIDI 插件性能对比：**

| 插件 | CPU 使用 | 内存 | 初始化时间 | 音质 |
|------|---------|------|-----------|------|
| FluidSynth | 5-10% | 20-50MB | 100-500ms | 优秀 |
| WildMIDI | 2-3% | 5-10MB | 10-50ms | 良好 |
| Timidity | 1-2% | 3-5MB | 10-50ms | 良好 |
| ADLMIDI | 1-2% | 2-3MB | <10ms | 良好 |
| OPNMIDI | 1-2% | 2-3MB | <10ms | 良好 |
| TinySoundFont | 2-4% | 5-15MB | 50-200ms | 良好 |

**选择建议：**

- **追求音质**：FluidSynth
- **平衡性能**：WildMIDI 或 TinySoundFont
- **最低资源**：ADLMIDI 或 OPNMIDI
- **快速初始化**：Timidity 或 ADLMIDI

---

## 示例程序

### 示例 1：基础混音器测试

演示多轨混音功能：

```bash
cd build/examples
./mixer_basic_test wav_samples/car_small.wav output.wav
```

**功能：**
- 加载单个音频文件
- 创建 4 个变化轨道（不同延迟、音量、音调）
- 混音 5 秒音频
- 输出 WAV 文件

### 示例 2：城市场景混音

使用 TOML 配置生成复杂城市环境音：

```bash
cd build/examples
./scene_city_test configs/city_scene.toml output_city.wav
```

**功能：**
- 加载 TOML 配置
- 随机生成多种城市音效（汽车、喇叭、鸟叫）
- 混音 30 秒
- 输出 WAV 文件

**配置示例（`city_scene.toml`）：**

```toml
[output]
format = "MONO16"
sample_rate = 44100

[scene]
duration = 30.0

[source.car]
wav_file = "wav_samples/car_small.wav"
min_count = 2
max_count = 4
min_interval = 1.0
max_interval = 3.0
min_volume = 0.6
max_volume = 1.0
min_pitch = 0.95
max_pitch = 1.05
```

### 示例 3：蜂群场景

高密度音源叠加示例：

```bash
cd build/examples
./scene_swarm_test configs/swarm_scene.toml output_swarm.wav
```

**功能：**
- 创建 10-15 个蜜蜂音效实例
- 不同音量和音调变化
- 模拟真实蜂群效果
- 混音 10 秒

### 示例 4：音频重采样工具

```bash
cd build/examples
./wav_resample input.wav output.wav 48000 sinc
```

**参数：**
- `input.wav`：输入文件
- `output.wav`：输出文件
- `48000`：目标采样率
- `sinc` 或 `linear`：重采样算法

### 准备示例音频

示例程序需要 WAV 音频文件，放在 `examples/wav_samples/` 目录：

```bash
cd examples/wav_samples
# 需要以下文件：
# - car_small.wav
# - car_suv.wav
# - car_truck.wav
# - horn_short.wav
# - horn_long.wav
# - brake_screech.wav
# - bird_chirp.wav
# - bee_buzz.wav
```

文件要求：
- 格式：单声道 WAV
- 位深：16-bit PCM
- 采样率：22050Hz 或 44100Hz

---

## API 参考

### AudioManager API

```cpp
class AudioManager
{
public:
    AudioManager(int count = 8);  // 创建指定数量的音源
    ~AudioManager();
    
    bool Play(const os_char* filename, float gain = 1.0f);
};
```

### AudioBuffer API

```cpp
class AudioBuffer
{
public:
    AudioBuffer(const os_char* filename = nullptr,
                AudioFileType type = AudioFileType::None);
    ~AudioBuffer();
    
    bool Load(const os_char* filename,
              AudioFileType type = AudioFileType::None);
    void Clear();
    
    uint GetIndex() const;
    double GetTime() const;     // 音频时长（秒）
    uint GetSize() const;       // 数据大小（字节）
    uint GetFreq() const;       // 采样率
};
```

### AudioSource API

```cpp
class AudioSource
{
public:
    AudioSource(bool create = false);
    AudioSource(AudioBuffer* buffer);
    ~AudioSource();
    
    // 播放控制
    bool Play();
    bool Play(bool loop);
    void Pause();
    void Resume();
    void Stop();
    void Rewind();
    
    // 绑定
    bool Link(AudioBuffer* buffer);
    void Unlink();
    
    // 音量音调
    void SetGain(float gain);
    float GetGain() const;
    void SetPitch(float pitch);
    float GetPitch() const;
    
    // 空间属性
    void SetPosition(const Vector3f& pos);
    const Vector3f& GetPosition() const;
    void SetVelocity(const Vector3f& vel);
    const Vector3f& GetVelocity() const;
    void SetDirection(const Vector3f& dir);
    const Vector3f& GetDirection() const;
    
    // 距离
    void SetDistance(float ref_distance, float max_distance);
    void SetRolloffFactor(float factor);
    float GetRolloffFactor() const;
    
    // 锥形
    void SetConeAngle(const ConeAngle& angle);
    const ConeAngle& GetAngle() const;
    void SetConeGain(float gain);
    float GetConeGain() const;
    
    // 滤波器
    bool SetLowpassFilter(float gain, float gain_hf);
    bool SetHighpassFilter(float gain, float gain_lf);
    bool SetBandpassFilter(float gain, float gain_lf, float gain_hf);
    void DisableFilter();
    
    // 状态查询
    int GetState() const;
    bool IsPlaying() const;
    bool IsPaused() const;
    bool IsStopped() const;
};
```

### AudioPlayer API

```cpp
class AudioPlayer : public Thread
{
public:
    AudioPlayer();
    ~AudioPlayer();
    
    // 加载
    bool Load(const os_char* filename,
              AudioFileType type = AudioFileType::None);
    
    // 播放控制
    void Play(bool loop = true);
    void Stop();
    void Pause();
    void Resume();
    
    // 循环
    bool IsLoop();
    void SetLoop(bool loop);
    
    // 音量音调
    void SetGain(float gain);
    float GetGain() const;
    void SetPitch(float pitch);
    float GetPitch() const;
    
    // 淡入淡出
    void SetFadeTime(PreciseTime fade_in, PreciseTime fade_out);
    void AutoGain(float target, PreciseTime duration, PreciseTime current_time);
    
    // 空间属性
    void SetPosition(const Vector3f& pos);
    const Vector3f& GetPosition() const;
    void SetVelocity(const Vector3f& vel);
    const Vector3f& GetVelocity() const;
    void SetDirection(const Vector3f& dir);
    const Vector3f& GetDirection() const;
    void SetDistance(float ref_distance, float max_distance);
    
    // 状态查询
    PlayState GetPlayState() const;
    PreciseTime GetPlayTime();
    double GetTotalTime() const;
};
```

### AudioMixer API

```cpp
namespace hgl::audio
{
    class AudioMixer
    {
    public:
        AudioMixer();
        ~AudioMixer();
        
        // 添加音源
        int AddSourceAudio(const void* data, uint size,
                          uint format, uint sampleRate);
        void ClearSources();
        int GetSourceCount() const;
        
        // 添加轨道
        void AddTrack(const MixingTrack& track);
        void AddTrack(uint sourceIndex, float timeOffset,
                     float volume, float pitch);
        void ClearTracks();
        int GetTrackCount() const;
        
        // 配置
        void SetConfig(const MixerConfig& config);
        const MixerConfig& GetConfig() const;
        void SetOutputFormat(uint format);
        uint GetOutputFormat() const;
        
        // 混音
        bool Mix(void** outputData, uint* outputSize,
                float loopLength = 0.0f);
        
        const AudioDataInfo& GetOutputInfo() const;
    };
    
    // 配置结构
    struct MixerConfig
    {
        bool useSoftClipper = false;  // 软削波
        bool useDither = false;       // 抖动
    };
    
    // 轨道结构
    struct MixingTrack
    {
        uint sourceIndex = 0;      // 音源索引
        float timeOffset = 0.0f;   // 时间偏移（秒）
        float volume = 1.0f;       // 音量 0.0-1.0
        float pitch = 1.0f;        // 音调 0.5-2.0
    };
}
```

### MIDIPlayer API

```cpp
class MIDIPlayer : public Thread
{
public:
    MIDIPlayer();
    ~MIDIPlayer();
    
    // 加载
    bool Load(const os_char* filename);
    
    // 播放控制
    void Play(bool loop = true);
    void Stop();
    void Pause();
    void Resume();
    
    // 音色库
    bool SetSoundFont(const std::string& path);
    bool SetBank(int bank_id);
    bool SetBankFile(const std::string& path);
    void SetSampleRate(int rate);
    
    // 通道控制
    bool SetChannelProgram(int channel, int program);
    bool SetChannelVolume(int channel, int volume);
    bool SetChannelPan(int channel, int pan);
    bool SetChannelMute(int channel, bool mute);
    bool SetChannelSolo(int channel, bool solo);
    void ResetChannel(int channel);
    void ResetAllChannels();
    
    // 查询
    bool GetChannelInfo(int channel, MidiChannelInfo* info);
    bool GetChannelActivity(int channel);
    
    // 多通道解码
    bool DecodeChannel(int channel, void** data, uint* size);
    bool DecodeAllChannels(void*** channels, uint* size);
    
    // 状态查询
    MIDIPlayState GetPlayState() const;
    PreciseTime GetPlayTime();
};
```

### MIDIOrchestraPlayer API

```cpp
class MIDIOrchestraPlayer : public Thread
{
public:
    MIDIOrchestraPlayer();
    ~MIDIOrchestraPlayer();
    
    // 加载
    bool Load(const os_char* filename);
    bool Load(io::InputStream* stream, int size = -1);
    
    // 播放控制
    void Play(bool loop = false);
    void Pause();
    void Resume();
    void Stop();
    void Close();
    
    // MIDI 配置
    void SetSoundFont(const char* path);
    void SetBank(int bank_id);
    void SetSampleRate(int rate);
    
    // 3D 空间布局
    void SetLayout(OrchestraLayout layout);
    void SetOrchestraCenter(const Vector3f& center);
    void SetOrchestraScale(float scale);
    
    void SetChannelPosition(int channel, const Vector3f& position);
    Vector3f GetChannelPosition(int channel) const;
    void SetChannelVolume(int channel, float volume);
    void SetChannelEnabled(int channel, bool enabled);
    AudioSource* GetChannelSource(int channel);
    
    // 通道控制
    void SetChannelProgram(int channel, int program);
    void MuteChannel(int channel, bool mute);
    void SoloChannel(int channel, bool solo);
    
    // 自动增益
    void AutoGain(float start_gain, float gap, float end_gain);
    void FadeIn(float gap);
    void FadeOut(float gap);
    
    // 状态查询
    bool IsNone() const;
    bool IsPlaying() const;
    bool IsPaused() const;
    int GetChannelCount();
    MidiChannelInfo GetChannelInfo(int channel);
};

// 预设布局枚举
enum class OrchestraLayout
{
    Standard,   // 标准交响乐团布局
    Chamber,    // 室内乐布局
    Jazz,       // 爵士乐布局
    Rock,       // 摇滚乐队布局
    Custom      // 自定义布局
};

// 通道位置配置
struct OrchestraChannelPosition
{
    Vector3f position;  // 3D 位置
    float gain;         // 音量增益 (0.0-1.0)
    bool enabled;       // 是否启用此通道
};
```

---

## 常见问题

### Q1：音频无声音输出

**可能原因：**
1. OpenAL 设备未正确初始化
2. 音源未绑定缓冲区
3. 音量设置为 0
4. 音频文件格式不支持

**解决方法：**

```cpp
// 检查 OpenAL 初始化
ALCdevice* device = alcGetContextsDevice(alcGetCurrentContext());
if(!device)
{
    std::cerr << "OpenAL device not initialized" << std::endl;
}

// 检查音源绑定
if(!source.Link(&buffer))
{
    std::cerr << "Failed to link buffer" << std::endl;
}

// 检查音量
source.SetGain(1.0f);

// 检查文件格式
AudioBuffer buffer(OS_TEXT("test.wav"), AudioFileType::WAV);
```

### Q2：MIDI 文件无法播放

**可能原因：**
1. MIDI 插件未安装
2. 音色库文件缺失
3. 音色库路径错误

**解决方法：**

```bash
# 安装 MIDI 插件（FluidSynth）
sudo apt-get install libfluidsynth-dev fluid-soundfont-gm

# 设置音色库路径
export FLUIDSYNTH_SF2=/usr/share/sounds/sf2/FluidR3_GM.sf2

# 检查音色库文件是否存在
ls -l /usr/share/sounds/sf2/FluidR3_GM.sf2
```

```cpp
// 代码中设置
MIDIPlayer player;
if(!player.SetSoundFont("/usr/share/sounds/sf2/FluidR3_GM.sf2"))
{
    std::cerr << "Failed to load SoundFont" << std::endl;
}
```

### Q3：混音输出削波失真

**原因：**
多个音轨叠加时，总音量超过最大值导致削波。

**解决方法：**

```cpp
// 方法 1：降低每个轨道音量
mixer.AddTrack(source1, 0.0f, 0.5f, 1.0f);  // 音量 50%
mixer.AddTrack(source2, 0.5f, 0.5f, 1.0f);  // 音量 50%

// 方法 2：启用软削波
MixerConfig config;
config.useSoftClipper = true;  // 平滑削波
mixer.SetConfig(config);

// 方法 3：使用浮点输出格式
mixer.SetOutputFormat(AL_FORMAT_MONO_FLOAT32);
```

### Q4：3D 音频方向不正确

**原因：**
听者（摄像机）朝向未设置。

**解决方法：**

```cpp
#include <hgl/audio/Listener.h>

// 设置听者位置
Listener::SetPosition(Vector3f(0.0f, 0.0f, 0.0f));

// 设置听者朝向（重要！）
Listener::SetOrientation(
    Vector3f(0.0f, 0.0f, -1.0f),  // 前方向（-Z轴）
    Vector3f(0.0f, 1.0f, 0.0f)    // 上方向（+Y轴）
);
```

### Q5：编译错误：找不到 OpenAL

**解决方法：**

```bash
# Ubuntu/Debian
sudo apt-get install libopenal-dev

# Fedora/CentOS
sudo dnf install openal-soft-devel

# macOS
brew install openal-soft

# Windows (vcpkg)
vcpkg install openal-soft
```

重新配置 CMake：

```bash
cd build
rm CMakeCache.txt
cmake ..
make
```

### Q6：音频播放卡顿

**可能原因：**
1. CPU 占用过高
2. 缓冲区太小
3. 磁盘 I/O 慢

**优化方法：**

```cpp
// 1. 预加载音频到内存
AudioBuffer buffer(OS_TEXT("bgm.ogg"));  // 提前加载

// 2. 降低 MIDI 质量（如果使用 FluidSynth）
player.SetSampleRate(22050);  // 降低采样率

// 3. 减少同时播放的音源数量
AudioManager manager(4);  // 只创建 4 个音源
```

### Q7：内存占用过高

**优化方法：**

```cpp
// 1. 使用流式播放（AudioPlayer）
AudioPlayer player;
player.Load(OS_TEXT("large_file.ogg"));  // 不会全部加载到内存

// 2. 及时释放不用的缓冲区
{
    AudioBuffer buffer(OS_TEXT("temp.wav"));
    // 使用...
}  // 自动释放

// 3. 共享音频缓冲区
AudioBuffer sharedBuffer(OS_TEXT("common.wav"));
AudioSource source1(&sharedBuffer);
AudioSource source2(&sharedBuffer);  // 共享缓冲区
```

### Q8：示例程序编译失败

**检查：**

```bash
# 确保启用了示例
cd build
cmake .. -DBUILD_EXAMPLES=ON
make

# 检查是否缺少示例音频文件
cd examples/wav_samples
ls -l *.wav
```

### Q9：如何调试音频问题

**调试技巧：**

```cpp
// 启用日志
#define ENABLE_AUDIO_LOG

// 检查 OpenAL 错误
ALenum error = alGetError();
if(error != AL_NO_ERROR)
{
    std::cerr << "OpenAL error: " << error << std::endl;
}

// 查询音频信息
AudioBuffer buffer(OS_TEXT("test.wav"));
std::cout << "Duration: " << buffer.GetTime() << "s" << std::endl;
std::cout << "Sample rate: " << buffer.GetFreq() << "Hz" << std::endl;
std::cout << "Size: " << buffer.GetSize() << " bytes" << std::endl;

// 查询音源状态
std::cout << "State: " << source.GetState() << std::endl;
std::cout << "Playing: " << source.IsPlaying() << std::endl;
```

---

## 附录

### A. 音频格式参考

#### 支持的 OpenAL 格式

| 常量 | 描述 |
|------|------|
| `AL_FORMAT_MONO8` | 单声道 8 位 |
| `AL_FORMAT_MONO16` | 单声道 16 位 |
| `AL_FORMAT_STEREO8` | 立体声 8 位 |
| `AL_FORMAT_STEREO16` | 立体声 16 位 |
| `AL_FORMAT_MONO_FLOAT32` | 单声道 32 位浮点 |
| `AL_FORMAT_STEREO_FLOAT32` | 立体声 32 位浮点 |

#### 文件格式枚举

```cpp
enum class AudioFileType
{
    None,       // 自动检测
    WAV,        // WAV 文件
    OGG,        // Ogg Vorbis
    Opus,       // Opus 编解码
    MIDI        // MIDI 文件
};
```

### B. 距离衰减模型

| 模型 | 常量 | 公式 |
|------|------|------|
| **无衰减** | `AL_NONE` | gain = 1.0 |
| **反比距离** | `AL_INVERSE_DISTANCE` | gain = ref / (ref + rolloff * (dist - ref)) |
| **反比距离限制** | `AL_INVERSE_DISTANCE_CLAMPED` | 同上，但限制在 [ref, max] |
| **线性距离** | `AL_LINEAR_DISTANCE` | gain = (1 - rolloff * (dist - ref) / (max - ref)) |
| **线性距离限制** | `AL_LINEAR_DISTANCE_CLAMPED` | 同上，限制在 [ref, max] |
| **指数距离** | `AL_EXPONENT_DISTANCE` | gain = (dist / ref) ^ (-rolloff) |
| **指数距离限制** | `AL_EXPONENT_DISTANCE_CLAMPED` | 同上，限制在 [ref, max] |

**推荐：**
- 室内：`AL_LINEAR_DISTANCE_CLAMPED`
- 室外：`AL_INVERSE_DISTANCE_CLAMPED`

### C. GM MIDI 乐器表（部分）

| 编号 | 乐器 | 编号 | 乐器 |
|------|------|------|------|
| 0 | 大钢琴 | 32 | 原声贝司 |
| 1 | 明亮钢琴 | 40 | 小提琴 |
| 4 | 电钢琴 | 48 | 弦乐合奏 |
| 16 | 打击型风琴 | 56 | 小号 |
| 24 | 原声吉他 | 64 | 高音萨克斯 |
| 25 | 电吉他 | 73 | 长笛 |

完整列表：[General MIDI 规范](https://www.midi.org/specifications)

### D. 性能优化建议

#### 音源管理

```cpp
// 优化：使用对象池
class AudioSourcePool
{
    std::vector<AudioSource*> pool;
    std::vector<AudioSource*> active;
    
public:
    AudioSource* Acquire()
    {
        if(pool.empty())
            return new AudioSource();
        
        auto source = pool.back();
        pool.pop_back();
        active.push_back(source);
        return source;
    }
    
    void Release(AudioSource* source)
    {
        source->Stop();
        active.erase(std::remove(active.begin(), active.end(), source),
                     active.end());
        pool.push_back(source);
    }
};
```

#### 缓冲区复用

```cpp
// 共享常用音效缓冲区
class AudioCache
{
    std::map<std::string, AudioBuffer*> cache;
    
public:
    AudioBuffer* GetBuffer(const std::string& name)
    {
        auto it = cache.find(name);
        if(it != cache.end())
            return it->second;
        
        auto buffer = new AudioBuffer(name.c_str());
        cache[name] = buffer;
        return buffer;
    }
};
```

### E. 相关资源

#### 官方文档

- [OpenAL 规范](https://www.openal.org/documentation/)
- [OpenAL EFX 扩展](https://openal.org/documentation/OpenAL_Programmers_Guide.pdf)
- [General MIDI 标准](https://www.midi.org/specifications)

#### 音频资源

- [FreeSound](https://freesound.org/) - 免费音效
- [OpenGameArt](https://opengameart.org/) - 游戏音频资源
- [MuseScore](https://musescore.org/) - MIDI 文件和乐谱

#### SoundFont 资源

- [FluidR3_GM](https://github.com/musescore/MuseScore/tree/master/share/sound) - 通用 SoundFont
- [GeneralUser GS](http://www.schristiancollins.com/generaluser.php) - 高质量 SoundFont
- [Timbres of Heaven](http://midkar.com/soundfonts/) - 专业级 SoundFont

#### 工具

- [Audacity](https://www.audacityteam.org/) - 音频编辑
- [LMMS](https://lmms.io/) - 音乐制作
- [MuseScore](https://musescore.org/) - MIDI 编辑

### F. 版本历史

详见 [ReleaseNotes.md](ReleaseNotes.md)

### G. 许可证

CMAudio 遵循项目仓库中的 LICENSE 文件规定的许可证。

### H. 贡献

欢迎贡献代码、报告 Bug 或提出功能建议：

- GitHub: https://github.com/hyzboy/CMAudio
- Issues: https://github.com/hyzboy/CMAudio/issues

---

## 结语

CMAudio 提供了完整的音频解决方案，从简单的音效播放到复杂的 3D 空间音频和专业 MIDI 播放。本手册涵盖了主要功能和常见使用场景，更多细节请参考头文件注释和示例程序。

如有问题或建议，欢迎在 GitHub 上提交 Issue 或 Pull Request。

**祝您使用愉快！**
