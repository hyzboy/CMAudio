---
title: "会话策略"
linkTitle: "会话策略"
weight: 140
date: 2026-08-15
description: "AudioSessionPolicy：iOS 静音开关 / Android Audio Focus 抽象"
draft: false
---

# 会话策略（AudioSessionPolicy）

`AudioSessionPolicy` 封装移动平台特有的音频会话行为，让游戏逻辑以统一接口处理"音频焦点"与"静音"：

```cpp
enum class AudioFocusState
{
    HasFocus,       // 拥有焦点，正常播放
    LostTransient,  // 暂时失去（来电/导航提示），恢复后自动继续
    Lost,           // 永久失去（切换到其它媒体），需显式重新请求
};

class AudioSessionPolicy
{
    virtual bool Initialize() = 0;            // 初始化
    virtual bool RequestFocus() = 0;          // 请求音频焦点（Android）
    virtual void AbandonFocus() = 0;          // 放弃音频焦点
    virtual void Update() = 0;                // 每帧轮询（iOS 静音开关状态）
    virtual AudioFocusState GetFocusState() const = 0;
    virtual bool IsSilenced() const = 0;      // 是否被静音（iOS 静音开关）
};

AudioSessionPolicy *CreateSessionPolicy();    // 按平台创建实现（调用方 delete）
```

## 平台差异

| 平台 | 行为 |
|---|---|
| iOS/iPadOS/tvOS | 硬件静音开关（Ring/Silent）检测 |
| Android | 音频焦点（Audio Focus）请求/放弃 |
| 桌面/主机 | 无焦点竞争、无静音开关（`DesktopSessionPolicy`） |

移动平台实现用 `#if HGL_OS == HGL_OS_iOS / HGL_OS_Android` 条件编译；桌面返回 `DesktopSessionPolicy`。

## 引擎集成

```cpp
AudioSessionPolicy *policy = CreateSessionPolicy();
policy->Initialize();
policy->RequestFocus();

// 播放前检查
if(policy->GetFocusState() != AudioFocusState::Lost && !policy->IsSilenced())
    PlaySound();

// 每帧轮询（iOS 静音开关状态变化需轮询）
while(running) {
    policy->Update();
    if(policy->GetFocusState() == AudioFocusState::LostTransient)
        PauseBGM();                      // 来电时暂停
    else if(policy->IsSilenced())
        MuteAll();                       // 静音开关打开
    else
        ResumeBGM();
}
```

## DesktopSessionPolicy（桌面默认）

无焦点竞争、无静音开关的桌面实现，同时作为状态机参考：

```cpp
class DesktopSessionPolicy : public AudioSessionPolicy
{
    // AbandonFocus 后为 Lost，RequestFocus 恢复为 HasFocus（用于测试与状态机验证）
};
```

完整行为示例见 `session_policy_test.cpp`。
