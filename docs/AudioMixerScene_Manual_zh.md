# AudioMixerScene 详尽介绍与使用说明

## 概述

`AudioMixerScene` 用于离线生成“复杂场景音频”。它会按配置从多类音源中生成多个实例，并把所有实例混合为一段新的单声道音频数据。该系统适合生成环境背景、群体声音、城市氛围等可重复使用的场景音频片段。

与 `AudioMixer` 相比，`AudioMixerScene` 主要提供：

- 多音源类别的统一管理。
- 实例数量、间隔、音量、音调的随机化。
- 简易滤波与简易混响(可选)。
- 统一输出格式与采样率。

## 设计要点

- 仅支持单声道输入与输出(含 `AL_FORMAT_MONO8`/`AL_FORMAT_MONO16`/`AL_FORMAT_MONO_FLOAT32`)。
- 输出采样率必须与源音频一致。
- 若启用效果(滤波/混响)，内部会强制使用 float 混音路径，最后再输出目标格式。
- 适合离线生成，不适用于实时流式处理。

## 关键类型

### AudioMixerSourceConfig

每一种“音源类别”的配置，决定该类音源在场景中的生成方式。

- `data`/`dataSize`/`format`/`sampleRate`：源音频数据与格式。
- `minCount`/`maxCount`：该类音源生成实例数量范围。
- `minInterval`/`maxInterval`：相邻实例的出现间隔范围(秒)。
- `minVolume`/`maxVolume`：实例音量范围。
- `minPitch`/`maxPitch`：实例音调范围。
- `filterConfig`：滤波器基础参数。
- `filterRandom`：滤波参数随机扰动范围。
- `reverb`：简易混响参数(含随机扰动)。

### ApplyAudioFilterPreset

辅助函数，用 `AudioFilterPreset` 快速填充 `AudioMixerSourceConfig::filterConfig`。

```cpp
void ApplyAudioFilterPreset(AudioMixerSourceConfig &config, AudioFilterPreset preset);
```

该函数只写入滤波“基准值”，随机扰动由 `filterRandom` 决定。

## 使用流程

1. 创建 `AudioMixerScene`。
2. 设置输出格式与采样率。
3. 为每个音源类别准备 `AudioMixerSourceConfig` 并调用 `AddSource`。
4. 调用 `GenerateScene` 生成音频数据。

## 示例代码

### 1. 基本使用

```cpp
#include <hgl/audio/AudioMixerScene.h>
#include <hgl/audio/AudioMixerSourceConfig.h>

using namespace hgl::audio;

AudioMixerScene scene;
scene.SetOutputFormat(AL_FORMAT_MONO16, 44100);

AudioMixerSourceConfig cfg;
cfg.data = wavData;
cfg.dataSize = wavSize;
cfg.format = AL_FORMAT_MONO16;
cfg.sampleRate = 44100;

cfg.minCount = 3;
cfg.maxCount = 6;
cfg.minInterval = 0.2f;
cfg.maxInterval = 1.0f;

cfg.minVolume = 0.6f;
cfg.maxVolume = 1.0f;
cfg.minPitch = 0.95f;
cfg.maxPitch = 1.05f;

scene.AddSource(OS_TEXT("car"), cfg);

void* outData = nullptr;
uint outSize = 0;
scene.GenerateScene(&outData, &outSize, 10.0f);
```

### 2. 使用滤波预设

```cpp
AudioMixerSourceConfig cfg;
// ...填充基础参数...
ApplyAudioFilterPreset(cfg, AudioFilterPreset::OldRadio);

// 可选：叠加轻微随机扰动
cfg.filterRandom.gain = 0.05f;
cfg.filterRandom.gain_hf = 0.05f;

scene.AddSource(OS_TEXT("radio"), cfg);
```

### 3. 启用简易混响(含随机扰动)

```cpp
AudioMixerSourceConfig cfg;
// ...填充基础参数...

cfg.reverb.enable = true;
cfg.reverb.delay_ms = 90.0f;
cfg.reverb.feedback = 0.35f;
cfg.reverb.mix = 0.25f;

cfg.reverb.delay_ms_rand = 10.0f;
cfg.reverb.feedback_rand = 0.05f;
cfg.reverb.mix_rand = 0.05f;
```

## 效果说明

### 简易滤波

内部使用一阶低通/高通，带通由“低通 + 高通”串联得到。该滤波为“近似模拟”，目的是在离线混音时获得足够的风格化效果，而非精确的声学建模。

### 简易混响

简易混响基于单延迟反馈结构，适合轻量级场景氛围模拟，不追求真实空间声学。

## 重要限制与行为说明

- 只支持单声道。
- 输出采样率必须与源采样率一致。
- 当启用滤波/混响时，内部混音会走 float 路径再转换输出。
- 随机参数在“每个实例”生成时单独采样。

## 常见问题

- Q: 为什么输出采样率必须与源采样率一致？
  - A: 当前实现不在 `AudioMixerScene` 内做重采样，以避免重复计算与复杂性。

- Q: 可以使用 `AudioFilterPreset` 但仍自定义参数吗？
  - A: 可以。先用 `ApplyAudioFilterPreset` 填充基准值，再手动微调 `filterConfig` 或设置 `filterRandom`。

## 使用场景范例

- 城市环境声：车辆、喇叭、人群、风声的随机叠加。
- 群体声音：昆虫群、雨点群、脚步声群。
- 机械环境：多台机组或设备的交错运行声。
- UI/提示音批量合成：离线批量生成不同层次的提示音。
