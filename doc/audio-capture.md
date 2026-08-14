---
title: "录音"
linkTitle: "录音"
weight: 50
date: 2026-08-15
description: "AudioCapture 封装 OpenAL 录音，用于语音聊天、语音指令等场景"
draft: false
---

# 录音（AudioCapture）

`AudioCapture` 封装 OpenAL 的 `alcCapture` 系列函数指针，
用于语音聊天、卡拉OK、语音指令等录音场景。

```cpp
class AudioCapture
{
    bool Open(uint sample_rate=44100, uint format=AL_FORMAT_MONO16, double buffer_seconds=1.0);
    void Close();   bool IsOpen() const;
    bool Start();   bool Stop();
    int  GetAvailableSamples() const;                  // 可取用的样本数（0=暂无）
    int  ReadSamples(void *buffer, int sample_count);  // 读样本，返回实际读取数
    uint GetSampleRate() const;  uint GetSampleFormat() const;  uint GetBufferSize() const;
};
```

## 典型用法

```cpp
AudioCapture capture;
if(!capture.Open())            // 无录音设备返回 false
    return;

capture.Start();               // 开始录音

// 每帧轮询读取
while(recording) {
    int avail = capture.GetAvailableSamples();
    if(avail > 0) {
        int16_t buf[4096];
        int n = capture.ReadSamples(buf, 4096);   // 实际读取 n 个样本
        Process(buf, n);
    }
}

capture.Stop();
capture.Close();
```

## 注意事项

- `Open()` 的 `format` 用 OpenAL 的 `AL_FORMAT_*`（如 `AL_FORMAT_MONO16`）。
- `buffer_seconds` 决定内部环形缓冲大小，越大越能容忍读取抖动，但延迟越高。
- 采样率/格式在 `Open()` 时固定，之后不可变。
- 无录音设备（`Open()` 返回 false）时，录音相关调用应被上层跳过。
