---
title: "重采样与混音"
linkTitle: "重采样与混音"
weight: 110
date: 2026-08-15
description: "AudioResampler 重采样、AudioMixer 离线多轨混音、AudioMixerScene 场景混音"
draft: false
---

# 重采样与混音

## 音频数据描述（AudioDataInfo）

纯 CPU 层用自有的 `AudioDataInfo` 描述 PCM 格式，不依赖 OpenAL 枚举：

```cpp
struct AudioDataInfo
{
    uint sample_rate;        // 采样率
    uint channels;          // 声道数 (1/2/4/6/7/8)
    uint bits_per_sample;   // 位深 (8/16/32)
    bool is_float;          // 是否浮点
    uint data_size;         // 数据大小（字节）
};
```

`channels + bits_per_sample + is_float` 三元组完全描述采样格式；`AL_FORMAT_*` 只在 OpenAL 边界转换。

## AudioResampler（重采样）

基于 libsamplerate，支持 mono/stereo/quad/5.1/6.1/7.1 布局（8/16/32 位整型与 float32）：

```cpp
enum class ResampleQuality { Linear, SincFastest, SincMedium, SincBest };

bool Resample(const void *inputData, uint inputSize,
              const AudioDataInfo &inputInfo,
              uint outputSampleRate,
              const AudioDataInfo &outputInfo,     // 可为空（字段=0 表示沿用输入）
              ResampleQuality quality,
              void **outputData, uint *outputSize); // 调用方负责 delete[] outputData

bool ResampleMono(...);   // Resample 的兼容别名
```

- 输入/输出声道数必须一致（重采样不重映射声道布局）。
- `outputInfo.sample_rate==0` 保留输入采样率；`channels==0` 沿用输入格式。
- 比率 `= outputSampleRate / inputSampleRate`，注意用独立输出采样率参数，而非 `outputInfo` 里的值。

```cpp
AudioDataInfo in = {22050, 2, 16, false, in_size};
AudioDataInfo out;                       // 全 0 = 沿用输入格式
void *out_data = nullptr;  uint out_size = 0;
Resample(in_pcm, in_size, in, 48000, out, ResampleQuality::SincMedium, &out_data, &out_size);
```

## AudioMixer（离线多轨混音）

`AudioMixer` 把多个音轨叠加混音成一份数据，内部全程 float 处理：

```cpp
class AudioMixer
{
    int  AddSourceAudio(const AudioDataInfo &info, const void *data);
    int  AddSourceAudio(const void *data, uint size, uint format, uint sample_rate);
    void ClearSources();   int GetSourceCount() const;

    void AddTrack(const MixingTrack &track);
    void AddTrack(uint source_index, float time_offset, float volume, float pitch);
    void ClearTracks();    int GetTrackCount() const;

    void SetConfig(const MixerConfig &cfg);   const MixerConfig &GetConfig() const;
    bool SetOutputFormat(const AudioDataInfo &info);

    ParametricEQ &GetEQ();   void ClearEQ();        // 混音输出前应用 EQ

    bool Mix(void **outputData, uint *outputSize, float loopLength = 0.0f);
};
```

```cpp
struct MixingTrack { uint source_index; float time_offset, volume, pitch; };

struct MixerConfig
{
    bool normalize = true;         // 归一化输出防溢出
    float master_volume = 1.0f;
    bool use_soft_clipper = false; // tanh 软削波
    bool use_dither = false;       // float→int16 TPDF 抖动
};
```

```cpp
AudioMixer mixer;
int src = mixer.AddSourceAudio(pcm, size, AL_FORMAT_MONO16, 44100);
mixer.AddTrack(src, 0.0f, 1.0f, 1.0f);      // 原始
mixer.AddTrack(src, 0.5f, 0.7f, 0.95f);     // 延迟+轻+慢
mixer.AddTrack(src, 1.2f, 0.6f, 1.05f);     // 更延迟+高音调

mixer.GetEQ().AddBand(BiquadType::Peaking, 1000.0f, 1.0f, 3.0f);  // 输出前 EQ

void *out = nullptr;  uint out_size = 0;
mixer.Mix(&out, &out_size, 5.0f);           // 混 5 秒
```

**处理链**：多音源必须统一格式/采样率（不在此重采样）→ 每轨变调（libsamplerate）→
按 `track.volume × master_volume` 叠加 → 应用 EQ → 软削波/归一化 → 抖动转换输出。

## AudioMixerScene（场景混音）

`AudioMixerScene` 是高级场景混音：管理多种音源类型，按配置生成随机实例（数量/间隔/音量/音高），
支持滤波随机扰动与简易混响，TOML 驱动：

```cpp
class AudioMixerScene
{
    void AddSource(const OSString &name, const AudioMixerSourceConfig &config);
    void RemoveSource(const OSString &name);
    void ClearSources();
    bool GenerateScene(void **outputData, uint *outputSize, float duration);
};
```

```cpp
struct AudioMixerSourceConfig
{
    const void *data;  AudioDataInfo info;
    uint min_count, max_count;          // 实例数量范围
    float min_interval, max_interval;   // 出现间隔（秒）
    float min_volume, max_volume;       // 音量随机
    float min_pitch, max_pitch;         // 音调随机
    AudioFilterConfig filter_config;    // 滤波基准 + 随机扰动
    SimpleReverbConfig reverb;          // 简易混响（延迟+反馈+干湿）
};
```

配合 `AudioFilterPreset`（`ApplyAudioFilterPreset`）给音源套用滤波预设。
场景示例见 `scene_city_test` / `scene_swarm_test`（TOML 配置在 `examples/configs/`）。
