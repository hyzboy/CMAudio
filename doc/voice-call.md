---
title: "语音通话链"
linkTitle: "语音通话链"
weight: 14
date: 2026-08-17
description: "VoiceCall 语音通话：实时录音 → 预处理（NS/AGC/VAD）→ Opus 编解码 → 抖动缓冲 → 播放"
draft: false
---

# 语音通话链

CMAudio 的语音通话链（P0-P4）把实时录音、DSP 预处理、Opus 编解码、抖动缓冲与
会话策略串成一条完整的语音通话/语音聊天管线，全部在模块内闭环，可在本机环回测试。

## 架构总览

```text
发送端                               接收端
┌──────────────────┐               ┌──────────────────────────┐
│ CaptureSource     │               │ JitterBuffer（抖动重排）  │
│ 录音设备/mock 合成 │               └───────────┬──────────────┘
└─────────┬────────┘                           │
          ▼                                    ▼
┌──────────────────┐               ┌──────────────────────────┐
│ VoicePreprocess   │               │ AudioCodec::Decode       │
│ NS → AGC → VAD    │               │ Opus 解码（丢包走 PLC）   │
└─────────┬────────┘               └───────────┬──────────────┘
          ▼                                    ▼
┌──────────────────┐               ┌──────────────────────────┐
│ AudioCodec::Encode│               │ 输出回调 / 播放           │
│ Opus 编码（ver=5  │               │（AudioPlayer / 自定目标） │
│  编码插件接口）    │               └──────────────────────────┘
└──────────────────┘
```

- **编码器是可插拔插件能力**：`AudioCodecPlugInInterface`（插件接口 ver=5）与浮点解码接口
  一样是"可选能力"——只有实现 ver=5 的插件（当前 Opus）才提供编码；主库零编码器依赖。
- **丢包隐藏（PLC）**：解码接口 `Decode(packet=nullptr)` 走 Opus PLC，接收端丢包不中断。

## 组件

| 组件 | 头文件 | 职责 |
|---|---|---|
| `CaptureSource` | `CaptureSource.h` | 录音设备封装（P0）：帧式读取、mock 模式 |
| `VoicePreprocess` | `VoicePreprocess.h` | NS 频谱门限降噪 → AGC 自动增益 → VAD 语音检测 |
| `AudioCodec` | `AudioCodec.h` | 编解码器 RAII 封装（一个对象=双向通道） |
| `JitterBuffer` | `JitterBuffer.h` | 接收端抖动缓冲：乱序重排/迟到丢弃/丢包标记 |
| `VoiceCall` | `VoiceCall.h` | 全链编排：内存环回 + 设备模式 + 会话策略联动 |

## 快速开始（设备模式）

```cpp
#include <hgl/audio/VoiceCall.h>
#include <hgl/audio/CaptureSource.h>

using namespace hgl::audio;

// 输出回调：解码后 PCM → 播放/网络发送
static void OnOutput(const float *pcm, uint frames, void *user)
{
    // 送到播放端 / 写入网络
}

CaptureSource capture;
capture.Open(16000, 20, AL_FORMAT_MONO16, /*use_mock=*/false);   // 真实录音
capture.Start();

VoiceCall call;
call.AttachCapture(&capture);                    // 发送端挂录音源
call.SetOutputCallback(OnOutput);                // 接收端输出回调
call.Start(OS_TEXT("Opus"), 16000, 1, 32000);    // 16k mono 32kbps

while(running)
{
    call.UpdateDevice();    // 每帧驱动：采集→预处理→编码→解码→回调
}

call.Stop();
```

无录音设备时 `Open(..., /*use_mock=*/true)` 用内部 1kHz 正弦合成源，
全链路（含编解码/PLC/会话策略）仍可验证。

## 会话策略联动

`VoiceCall` 可选挂载 `AudioSessionPolicy`（移动平台焦点抽象）：

- `Start()` 自动 `RequestFocus()`；`Stop()` 自动 `AbandonFocus()`
- `UpdateDevice()` 每帧检查：**失焦/被静音 → 输出静音帧（通话挂起），恢复后自动继续**

```cpp
DesktopSessionPolicy policy;        // 桌面实现；移动端 CreateSessionPolicy()
policy.Initialize();

VoiceCall call;
call.SetSessionPolicy(&policy);
call.Start(OS_TEXT("Opus"), 16000, 1, 32000);
```

## 参数建议

| 参数 | 值 | 说明 |
|---|---|---|
| 采样率 | 16000 Hz | 语音带宽内，Opus 最优工作点 |
| 帧长 | 20 ms（320 样本） | Opus 标准帧长，延迟与压缩比平衡 |
| 码率 | 24-32 kbps | 语音质量良好（Opus 最低 6kbps 仍可懂） |
| 抖动缓冲 | 5 帧（100ms） | 容忍网络抖动；过大增加延迟 |

延迟预算（本机）：采集 20ms + 编码 20ms + 抖动缓冲 100ms + 播放队列 ≈ 150-200ms，
满足语音通话要求。

## 测试

| 测试 | 覆盖 |
|---|---|
| `audio_codec_test` | 编码插件接口加载、编解码环回主频、PLC、48kHz |
| `voice_preprocess_test` | AGC 电平归一、VAD 静音/语音/hangover、NS 降噪（噪声带能量降 45%） |
| `voice_call_test` | 抖动缓冲乱序/丢包/迟到、全链环回（无丢包/10% 丢包/噪声环境） |
| `voice_call_device_test` | 设备模式（mock/真实录音）、会话策略焦点挂起与恢复 |

> 运行需 `CMP.Audio.Opus.dll` 在运行目录（编码接口 ver=5 由 Opus 插件提供）。
