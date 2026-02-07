# AudioMixer 详尽介绍与使用说明

## 概述

`AudioMixer` 用于将同一段源音频叠加为多条轨道，并混合成一段新的音频数据。它支持单声道音频的时间偏移、音量、音调变化与采样率转换，并提供归一化、软削波和抖动等混音后处理能力。混音过程内部统一为 float 处理，避免早期溢出与失真。

本说明基于当前实现行为，强调可用能力与边界条件。

## 设计要点

- 仅支持单声道输入（AL_FORMAT_MONO8 / MONO16 / MONO_FLOAT32）。
- 内部统一转为 float 混音，最后再转换为目标输出格式。
- 变速变调与采样率转换均为线性插值重采样（质量优先于性能的基础实现）。
- 支持：归一化（normalize）、软削波（soft clipper）、TPDF 抖动（dither）。
- 输出格式主推 `AL_FORMAT_MONO16` 与 `AL_FORMAT_MONO_FLOAT32`（实现中亦支持 MONO8 输出）。
 - 设计目标是“多音源混为单一音源”，最终由整体 3D 音频系统统一空间化处理。

## 关键类型

### MixingTrack

每一条混音轨道的参数：

- `sourceIndex`：音源索引，指向混音器内部的音源列表。
- `timeOffset`：时间偏移，单位秒。
- `volume`：音量比例，范围建议 $[0.0, 1.0]$。
- `pitch`：音调变化，范围建议 $[0.5, 2.0]$；1.0 为原速原调。

### MixerConfig

混音器配置：

- `normalize`：归一化输出，防止峰值超过 1.0 造成硬削波。
- `masterVolume`：总音量缩放。
- `useSoftClipper`：启用软削波（tanh 曲线），更平滑的削波方式。
- `useDither`：float32 -> int16 时启用 TPDF 抖动以减少量化噪声。

### AudioDataInfo

内部用于描述源音频/输出音频的格式、采样率、位深等信息，当前固定为单声道。

## 使用流程

1. 创建 `AudioMixer`。
2. 通过 `AddSourceAudio` 添加一个或多个音源。
3. 通过 `AddTrack` 添加多个 `MixingTrack`。
4. 可选：设置 `MixerConfig`，以及输出格式。
5. 调用 `Mix` 生成新的音频数据。

## 重要限制与行为说明

- 只支持单声道输入。
- 本混音器不承担 3D 空间化职责，3D 位置与空间效果应由外部 3D 音频系统处理。
- 多个音源必须统一格式与采样率，不在混音器内部进行重采样。
- `pitch` 变化使用线性插值，音质在大幅变速时会有损失。
- 若启用 `useSoftClipper`，则不会进行归一化。
- 若关闭归一化与软削波，超出 $[-1, 1]$ 的样本在输出转换时会硬削波。
- 输出采样率固定为音源采样率。

## 输出长度计算

当 `Mix` 传入 `loopLength <= 0` 时，混音器会根据所有轨道的结束时间计算输出长度：

- 源音频时长：$duration = samples / sampleRate$。
- 轨道结束时间：$timeOffset + duration / pitch$。
- 输出长度取所有轨道结束时间的最大值。

## 混音处理细节

### 流程概览

1. 源数据转换为 float。
2. 对每条轨道执行：
   - 音调变化（线性插值重采样）。
   - 根据 `timeOffset` 写入到输出缓冲并叠加。
3. 后处理：软削波或归一化。
4. 转换为输出格式，得到最终数据。

### 软削波

使用 $tanh(x)$ 进行软削波，保证输出在 $[-1, 1]$，减少刺耳失真。

### 归一化

仅在 `useSoftClipper == false` 时可用。若峰值超过 1.0，则整体按峰值缩放。

### 抖动

仅在 float32 -> int16 转换时生效，使用 TPDF 噪声，可减少量化失真。

## 示例代码

### 基本使用

```cpp
#include <hgl/audio/AudioMixer.h>

using namespace hgl::audio;

AudioMixer mixer;

const void* srcData = ...;
uint srcSize = ...;
uint srcFormat = AL_FORMAT_MONO16;
uint srcSampleRate = 44100;

int sourceIndex = mixer.AddSourceAudio(srcData, srcSize, srcFormat, srcSampleRate);
if(sourceIndex < 0)
{
  return;
}

mixer.AddTrack(sourceIndex, 0.0f, 1.0f, 1.0f);
mixer.AddTrack(sourceIndex, 0.5f, 0.8f, 0.95f);
mixer.AddTrack(sourceIndex, 1.2f, 0.6f, 1.05f);

MixerConfig cfg;
cfg.normalize = true;
cfg.masterVolume = 0.9f;

mixer.SetConfig(cfg);

void* outData = nullptr;
uint outSize = 0;

if(mixer.Mix(&outData, &outSize))
{
    // 使用 outData / outSize
    delete[] (char*)outData;
}
```

### 浮点输出

```cpp
mixer.SetOutputFormat(AL_FORMAT_MONO_FLOAT32);

void* outData = nullptr;
uint outSize = 0;

if(mixer.Mix(&outData, &outSize))
{
    // outData 是 float32 数据
    delete[] (float*)outData;
}
```

### 启用软削波与抖动

```cpp
MixerConfig cfg;
cfg.useSoftClipper = true;
cfg.normalize = false;   // soft clipper 会替代归一化
cfg.useDither = true;    // float32 -> int16 时生效

mixer.SetConfig(cfg);
```

## 常见问题

- Q: 为什么只支持单声道？
  - A: 当前实现定位为轻量混音器，暂未扩展多声道混音逻辑。

- Q: 归一化与软削波可以同时使用吗？
  - A: 当前实现二者互斥，启用软削波时会跳过归一化。

## 使用场景范例

### 1. 环境循环音的层叠

将同一段循环素材叠加为多条轨道，并设置不同的时间偏移与轻微的音调差异，让环境声更自然。

### 2. 森林环境的多源合成

开发者提供多种鸟叫、水流声、风声、树木摇晃等音效，但全部作为 3D 单体声源会过多。可先用混音器叠加为单一“森林背景音”，再交给整体 3D 系统进行空间化播放。

### 3. 多物体同类音效聚合

将同类型音效（例如多辆车、多台机器）混成一条音源，降低 3D 声源数量，提高整体效率。

### 4. 交通噪声背景合成

将多种汽车的轰鸣、刹车、鸣笛等声音，按时间偏移和音量比例叠加，合成“多辆汽车同时行进”的嘈杂交通背景音。

### 5. 远近层次的模拟

通过不同轨道的音量与延迟模拟远近层次，再由 3D 音频系统统一空间化处理。

### 6. UI 音效的批量合成

将多个 UI 点击音效在离线阶段混成一个短片段，减少运行时频繁触发。

### 7. 组合式音效设计

使用 1 个基础音源，通过多个轨道的时间错位、音调差异，生成“合成后的新音效”。

### 8. 群体脚步声

将不同节奏与力度的脚步声叠加，合成“人群行进”的背景声，避免大量实体脚步声源。

### 9. 多设备噪声背景

将多台设备的风扇声、硬盘声、电流噪声等叠加，形成“机房/工作间”的连续氛围音。

### 10. 天气与灾害背景

叠加风声、雨声、雷声、树木摇晃等素材，合成风暴或暴雨背景音，再由 3D 系统统一控制方向与距离。

## 注意事项与建议

- `pitch` 与 `outputSampleRate` 同时改变会叠加变速效果，建议逐一调整验证音质。
- 若你追求更高音质，可考虑后续引入更高阶插值或重采样算法。
- 多轨混音可能造成峰值明显上升，建议开启 `normalize` 或 `useSoftClipper`。
