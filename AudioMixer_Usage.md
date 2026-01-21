# Audio Mixer 使用说明

## 概述

AudioMixer 是一个简单的音频混音器，用于将多个音轨混合成一个音频流。它支持以下功能：

* 时间偏移 - 控制每个音轨开始播放的时间
* 音量控制 - 为每个音轨设置独立的音量
* 音调变化 - 简单的音调变换（通过重采样实现）
* 循环音频混音 - 适用于循环音效的叠加

## 使用示例

### 基本使用

```cpp
#include <hgl/audio/AudioMixer.h>

using namespace hgl::audio;

// 创建混音器
AudioMixer mixer;

// 设置源音频数据
// 假设我们有一个汽车前进的循环音效
const void* carSoundData = ...; // 音频数据
uint dataSize = ...; // 数据大小
uint format = AL_FORMAT_MONO16; // 音频格式
uint sampleRate = 44100; // 采样率

mixer.SetSourceAudio(carSoundData, dataSize, format, sampleRate);

// 添加多个轨道模拟多辆汽车
// 第一辆车：原始音效
mixer.AddTrack(0.0f, 1.0f, 1.0f); // 时间偏移0秒，音量100%，原始音调

// 第二辆车：稍微延迟，音量小一点，音调稍低
mixer.AddTrack(0.5f, 0.8f, 0.95f); // 时间偏移0.5秒，音量80%，音调降低5%

// 第三辆车：更多延迟，音量更小，音调更高
mixer.AddTrack(1.2f, 0.6f, 1.05f); // 时间偏移1.2秒，音量60%，音调升高5%

// 第四辆车：远处的车
mixer.AddTrack(2.0f, 0.4f, 0.9f); // 时间偏移2秒，音量40%，音调降低10%

// 执行混音
void* outputData = nullptr;
uint outputSize = 0;
float loopLength = 5.0f; // 5秒的循环

if(mixer.Mix(&outputData, &outputSize, loopLength))
{
    // 混音成功，outputData 包含混音后的数据
    // 可以将这个数据加载到 AudioBuffer 中播放
    
    // 使用完后记得释放内存
    delete[] (char*)outputData;
}
```

### 配置混音器

```cpp
// 创建混音器配置
MixerConfig config;
config.normalize = true;        // 开启归一化，防止溢出
config.masterVolume = 0.8f;     // 主音量80%

mixer.SetConfig(config);
```

### 使用结构体添加轨道

```cpp
// 使用 MixingTrack 结构体
MixingTrack track;
track.timeOffset = 1.5f;    // 时间偏移1.5秒
track.volume = 0.7f;        // 音量70%
track.pitch = 1.1f;         // 音调升高10%

mixer.AddTrack(track);
```

## 参数说明

### MixingTrack 参数

* **timeOffset**: 时间偏移（秒）
  - 范围：>= 0.0
  - 控制音轨开始播放的时间点
  
* **volume**: 音量
  - 范围：0.0 - 1.0
  - 0.0 = 静音，1.0 = 原始音量
  
* **pitch**: 音调
  - 范围：0.5 - 2.0（推荐）
  - 1.0 = 原始音调
  - < 1.0 = 降低音调（变慢）
  - > 1.0 = 升高音调（变快）

### MixerConfig 参数

* **normalize**: 是否归一化
  - 开启后会对混音结果进行归一化处理，防止溢出
  
* **masterVolume**: 主音量
  - 范围：0.0 - 1.0
  - 应用于所有混音后的最终输出

## 支持的音频格式

* AL_FORMAT_MONO8 - 8位单声道
* AL_FORMAT_MONO16 - 16位单声道
* AL_FORMAT_STEREO8 - 8位立体声
* AL_FORMAT_STEREO16 - 16位立体声

## 注意事项

1. 音调变化使用简单的线性插值重采样，可能会在极端情况下产生一些音质损失
2. 时间偏移越大，输出的音频长度就越长
3. 如果不指定循环长度，混音器会自动计算所需的最小长度
4. 输出的音频数据需要调用者负责释放内存
5. 混音后的数据可以通过 AudioBuffer::SetData() 加载用于播放

## 应用场景

* 创建多个相似声音的叠加效果（如多辆汽车、多个脚步声等）
* 为循环音效添加变化，使其听起来更自然
* 实时音频混音和处理
* 游戏音效系统
