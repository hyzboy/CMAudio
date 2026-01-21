# AudioScene 使用说明

## 概述

AudioScene 是一个高级音频场景混音器，用于管理和混合多种音频源。它可以控制每种音源的数量、出现频率、变化幅度等，适合创建复杂的音频环境（如城市街道、森林等）。

## 主要特性

* **多音源管理** - 同时管理多种不同的音频源
* **数量控制** - 为每种音源设置最小和最大生成数量
* **出现频率** - 控制每种音源实例之间的时间间隔
* **变化范围** - 为每种音源设置音量和音调的随机变化范围
* **随机生成** - 自动随机生成符合参数的音频实例

## 核心数据结构

### AudioSourceConfig

音频源配置结构，定义单个音频源的特性和生成参数：

```cpp
struct AudioSourceConfig
{
    // 音频数据
    const void* data;           // 音频数据指针
    uint dataSize;              // 数据大小
    uint format;                // 音频格式 (AL_FORMAT_*)
    uint sampleRate;            // 采样率
    
    // 生成控制参数
    uint minCount;              // 最小生成数量
    uint maxCount;              // 最大生成数量
    float minInterval;          // 最小出现间隔(秒)
    float maxInterval;          // 最大出现间隔(秒)
    
    // 变化范围参数
    float minVolume;            // 最小音量(0.0-1.0)
    float maxVolume;            // 最大音量(0.0-1.0)
    float minPitch;             // 最小音调(0.5-2.0)
    float maxPitch;             // 最大音调(0.5-2.0)
};
```

## 使用示例

### 基本使用 - 城市街道场景

```cpp
#include <hgl/audio/AudioScene.h>

using namespace hgl::audio;

// 创建场景混音器
AudioScene scene;

// 配置小轿车音源
AudioSourceConfig carConfig;
carConfig.data = carSoundData;
carConfig.dataSize = carSoundSize;
carConfig.format = AL_FORMAT_MONO16;
carConfig.sampleRate = 44100;
carConfig.minCount = 2;          // 至少2辆车
carConfig.maxCount = 4;          // 最多4辆车
carConfig.minInterval = 1.0f;    // 最小间隔1秒
carConfig.maxInterval = 3.0f;    // 最大间隔3秒
carConfig.minVolume = 0.6f;      // 音量范围60%-100%
carConfig.maxVolume = 1.0f;
carConfig.minPitch = 0.95f;      // 音调范围95%-105%
carConfig.maxPitch = 1.05f;

scene.AddSource(OS_TEXT("小轿车"), carConfig);

// 配置SUV音源
AudioSourceConfig suvConfig;
suvConfig.data = suvSoundData;
suvConfig.dataSize = suvSoundSize;
suvConfig.format = AL_FORMAT_MONO16;
suvConfig.sampleRate = 44100;
suvConfig.minCount = 1;
suvConfig.maxCount = 2;
suvConfig.minInterval = 2.0f;
suvConfig.maxInterval = 5.0f;
suvConfig.minVolume = 0.7f;
suvConfig.maxVolume = 0.9f;
suvConfig.minPitch = 0.9f;
suvConfig.maxPitch = 1.0f;

scene.AddSource(OS_TEXT("SUV"), suvConfig);

// 配置喇叭音源
AudioSourceConfig hornConfig;
hornConfig.data = hornSoundData;
hornConfig.dataSize = hornSoundSize;
hornConfig.format = AL_FORMAT_MONO16;
hornConfig.sampleRate = 44100;
hornConfig.minCount = 0;         // 可能不出现
hornConfig.maxCount = 3;         // 最多3次喇叭声
hornConfig.minInterval = 5.0f;   // 喇叭之间间隔较长
hornConfig.maxInterval = 10.0f;
hornConfig.minVolume = 0.8f;
hornConfig.maxVolume = 1.0f;
hornConfig.minPitch = 0.95f;
hornConfig.maxPitch = 1.1f;

scene.AddSource(OS_TEXT("喇叭"), hornConfig);

// 生成场景（持续30秒）
void* outputData = nullptr;
uint outputSize = 0;
uint outputFormat = 0;
uint outputSampleRate = 0;

if(scene.GenerateScene(&outputData, &outputSize, 
                       &outputFormat, &outputSampleRate, 30.0f))
{
    // 场景生成成功，可以播放或保存
    // outputData 包含混合后的音频数据
    
    // 使用完后释放内存
    delete[] (char*)outputData;
}
```

### 高级示例 - 完整的城市环境

```cpp
AudioScene cityScene;

// 设置全局配置
MixerConfig globalConfig;
globalConfig.normalize = true;
globalConfig.masterVolume = 0.9f;
cityScene.SetGlobalConfig(globalConfig);

// 1. 小轿车（常见，数量多）
AudioSourceConfig carConfig;
carConfig.data = carData;
carConfig.dataSize = carSize;
carConfig.format = AL_FORMAT_MONO16;
carConfig.sampleRate = 44100;
carConfig.minCount = 3;
carConfig.maxCount = 6;
carConfig.minInterval = 0.5f;
carConfig.maxInterval = 2.0f;
carConfig.minVolume = 0.5f;
carConfig.maxVolume = 0.9f;
carConfig.minPitch = 0.95f;
carConfig.maxPitch = 1.05f;
cityScene.AddSource(OS_TEXT("小轿车"), carConfig);

// 2. SUV（较少，音量较大）
AudioSourceConfig suvConfig;
suvConfig.data = suvData;
suvConfig.dataSize = suvSize;
suvConfig.format = AL_FORMAT_MONO16;
suvConfig.sampleRate = 44100;
suvConfig.minCount = 1;
suvConfig.maxCount = 3;
suvConfig.minInterval = 2.0f;
suvConfig.maxInterval = 4.0f;
suvConfig.minVolume = 0.7f;
suvConfig.maxVolume = 1.0f;
suvConfig.minPitch = 0.9f;
suvConfig.maxPitch = 1.0f;
cityScene.AddSource(OS_TEXT("SUV"), suvConfig);

// 3. 大卡车（偶尔，音调低）
AudioSourceConfig truckConfig;
truckConfig.data = truckData;
truckConfig.dataSize = truckSize;
truckConfig.format = AL_FORMAT_MONO16;
truckConfig.sampleRate = 44100;
truckConfig.minCount = 0;
truckConfig.maxCount = 2;
truckConfig.minInterval = 8.0f;
truckConfig.maxInterval = 15.0f;
truckConfig.minVolume = 0.8f;
truckConfig.maxVolume = 1.0f;
truckConfig.minPitch = 0.8f;
truckConfig.maxPitch = 0.95f;
cityScene.AddSource(OS_TEXT("大卡车"), truckConfig);

// 4. 自行车（轻快，音量小）
AudioSourceConfig bikeConfig;
bikeConfig.data = bikeData;
bikeConfig.dataSize = bikeSize;
bikeConfig.format = AL_FORMAT_MONO16;
bikeConfig.sampleRate = 44100;
bikeConfig.minCount = 1;
bikeConfig.maxCount = 4;
bikeConfig.minInterval = 1.0f;
bikeConfig.maxInterval = 3.0f;
bikeConfig.minVolume = 0.3f;
bikeConfig.maxVolume = 0.6f;
bikeConfig.minPitch = 1.0f;
bikeConfig.maxPitch = 1.15f;
cityScene.AddSource(OS_TEXT("自行车"), bikeConfig);

// 5. 刹车声（偶尔）
AudioSourceConfig brakeConfig;
brakeConfig.data = brakeData;
brakeConfig.dataSize = brakeSize;
brakeConfig.format = AL_FORMAT_MONO16;
brakeConfig.sampleRate = 44100;
brakeConfig.minCount = 0;
brakeConfig.maxCount = 2;
brakeConfig.minInterval = 10.0f;
brakeConfig.maxInterval = 20.0f;
brakeConfig.minVolume = 0.6f;
brakeConfig.maxVolume = 0.9f;
brakeConfig.minPitch = 0.95f;
brakeConfig.maxPitch = 1.05f;
cityScene.AddSource(OS_TEXT("刹车"), brakeConfig);

// 6. 喇叭声（随机）
AudioSourceConfig hornConfig;
hornConfig.data = hornData;
hornConfig.dataSize = hornSize;
hornConfig.format = AL_FORMAT_MONO16;
hornConfig.sampleRate = 44100;
hornConfig.minCount = 0;
hornConfig.maxCount = 3;
hornConfig.minInterval = 5.0f;
hornConfig.maxInterval = 12.0f;
hornConfig.minVolume = 0.7f;
hornConfig.maxVolume = 1.0f;
hornConfig.minPitch = 0.95f;
hornConfig.maxPitch = 1.1f;
cityScene.AddSource(OS_TEXT("喇叭"), hornConfig);

// 7. 鸟鸣（背景）
AudioSourceConfig birdConfig;
birdConfig.data = birdData;
birdConfig.dataSize = birdSize;
birdConfig.format = AL_FORMAT_MONO16;
birdConfig.sampleRate = 44100;
birdConfig.minCount = 2;
birdConfig.maxCount = 5;
birdConfig.minInterval = 3.0f;
birdConfig.maxInterval = 8.0f;
birdConfig.minVolume = 0.3f;
birdConfig.maxVolume = 0.6f;
birdConfig.minPitch = 0.9f;
birdConfig.maxPitch = 1.2f;
cityScene.AddSource(OS_TEXT("鸟鸣"), birdConfig);

// 生成60秒的城市环境音
void* sceneData = nullptr;
uint sceneSize = 0;

if(cityScene.GenerateScene(&sceneData, &sceneSize, 60.0f))
{
    // 场景生成成功
    // 可以保存为文件或直接播放
    
    delete[] (char*)sceneData;
}
```

## 参数调整建议

### 数量控制 (minCount / maxCount)

* **背景持续音**（如汽车引擎）：minCount = 2-5, maxCount = 5-10
* **常见事件**（如自行车）：minCount = 1-2, maxCount = 3-6
* **偶发事件**（如喇叭、刹车）：minCount = 0, maxCount = 1-3
* **稀少事件**（如大卡车）：minCount = 0, maxCount = 1-2

### 出现间隔 (minInterval / maxInterval)

* **密集**：0.5s - 2s（如城市高峰期汽车）
* **常规**：2s - 5s（如正常交通）
* **稀疏**：5s - 15s（如偶尔的喇叭声）
* **罕见**：15s - 30s（如远处火车）

### 音量范围 (minVolume / maxVolume)

* **主要音源**：0.7 - 1.0（如近处车辆）
* **次要音源**：0.5 - 0.8（如中等距离）
* **背景音源**：0.3 - 0.6（如远处声音、鸟鸣）
* **环境音**：0.2 - 0.4（如风声、远处人声）

### 音调范围 (minPitch / maxPitch)

* **自然变化**：0.95 - 1.05（模拟自然差异）
* **较大变化**：0.9 - 1.1（更多样化）
* **降低音调**：0.8 - 0.95（如大型车辆）
* **提高音调**：1.0 - 1.2（如小型车辆、鸟类）

## 与 AudioMixer 的关系

* **AudioMixer** - 低级混音器，处理单一音源的多轨混音
* **AudioScene** - 高级场景管理器，管理多种音源并自动生成变化

AudioScene 内部使用多个 AudioMixer 实例来实现复杂的混音效果。

## 注意事项

1. 所有音频源应使用相同的格式和采样率以获得最佳效果
2. 输出数据需要调用者负责释放内存（使用 delete[]）
3. 数量和间隔参数会影响场景的复杂度和真实感
4. 建议先用较短的持续时间测试参数效果
5. 场景生成是随机的，每次调用会产生不同的结果

## 应用场景

* 游戏环境音效（城市街道、森林、战场等）
* 影视后期音效制作
* 虚拟现实环境音
* 音频样本库创建
* 环境声音模拟
