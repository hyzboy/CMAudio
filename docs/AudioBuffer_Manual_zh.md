# AudioBuffer 技术手册

## 概述

`AudioBuffer` 是用于管理音频数据缓冲的轻量类，负责加载、保存与提供基础信息(时长/大小/采样率)。它适合用于一次性加载音频数据，再由 `AudioSource` 绑定进行播放。

## 主要特性

- 支持从内存、文件、输入流加载音频数据。
- 存储并提供音频数据的基础元信息(时长、大小、采样率)。
- 可被 `AudioSource` 绑定用于播放。

## 关键接口

### 构造与加载

- `AudioBuffer(void*, int, AudioFileType)`：从内存加载。
- `AudioBuffer(InputStream*, int, AudioFileType)`：从流加载。
- `AudioBuffer(const os_char*, AudioFileType)`：从文件加载。
- `Load(...)`：重新加载音频数据。

### 数据设置

- `SetData(uint format, const void* data, uint size, uint freq)`：手动设置音频数据。

### 查询接口

- `GetIndex()`：OpenAL 缓冲区索引。
- `GetTime()`：可播放时长(秒)。
- `GetSize()`：数据总字节数。
- `GetFreq()`：采样率(Hz)。

## 使用流程

1. 创建 `AudioBuffer`。
2. 使用 `Load` 或构造函数加载音频数据。
3. 使用 `AudioSource::Link()` 绑定并播放。

## 示例代码

```cpp
#include <hgl/audio/AudioBuffer.h>
#include <hgl/audio/AudioSource.h>

using namespace hgl;

AudioBuffer buffer(OS_TEXT("click.wav"), AudioFileType::Wav);
AudioSource source(&buffer);
source.Play();
```

## 注意事项

- `AudioBuffer` 只负责存储与加载，不处理空间化或效果。
- 输出格式需与加载器一致，避免格式不匹配。
