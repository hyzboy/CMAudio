# CMAudio 移动平台策略（P3-3）

> 本文描述 CMAudio 在移动平台的音频会话策略：iOS 静音开关与 Android 音频焦点。
> 对应代码：`inc/hgl/audio/AudioSessionPolicy.h` + `src/AudioSessionPolicy.cpp`。

## 一、概述

桌面/主机平台没有"音频会话"概念——应用独占音频输出，无焦点竞争、无静音开关。
移动平台则不同，操作系统对音频有全局管控：

| 平台 | 管控机制 | 引擎需响应 |
|------|----------|-----------|
| iOS/iPadOS/tvOS | 硬件静音开关（Ring/Silent switch） | 检测是否被静音 |
| Android | 音频焦点（Audio Focus） | 请求/放弃焦点，响应焦点被夺走 |

`AudioSessionPolicy` 抽象这两类行为，提供统一接口，桌面实现为无操作默认。

## 二、iOS/iPadOS/tvOS：静音开关策略

### 2.1 机制

iOS 设备侧边有**硬件静音开关**（Ring/Silent）。其行为由 `AVAudioSession` 分类决定：

| AVAudioSession 分类 | 静音开关开启时的行为 | 典型用途 |
|--------------------|---------------------|---------|
| `AVAudioSessionCategoryAmbient` | **静音**（遵循开关） | 非必要音效、提示音 |
| `AVAudioSessionCategoryPlayback` | **不静音**（忽略开关，继续播放） | 音乐、游戏、视频 |
| `AVAudioSessionCategorySoloAmbient`（默认） | 静音 | 通用音效 |

### 2.2 策略建议（游戏）

- 游戏是**主动媒体消费**场景，应使用 `AVAudioSessionCategoryPlayback`，让音乐/音效在静音开关开启时仍播放（与绝大多数游戏一致）。
- 若产品要求"跟随静音开关"（如休闲游戏），改用 `Ambient` 分类，并轮询开关状态。
- `IsSilenced()` 通过轮询实现（iOS 无静音开关状态的通知 API，需每帧或周期性查询）。

### 2.3 集成要点

```cpp
// 启动
session->Initialize();          // 内部设置 AVAudioSessionCategoryPlayback

// 每帧
session->Update();              // 轮询静音开关状态
if(session->IsSilenced())       // 静音时暂停/降音量
    engine->SetMasterMute(true);
```

## 三、Android：Audio Focus 策略

### 3.1 机制

Android 通过 `AudioManager.requestAudioFocus()` 管理多应用间的音频播放权。
当电话、导航、其它音乐应用需要发声时，系统会**夺走**当前应用的焦点。

焦点变化回调（`AudioManager.OnAudioFocusChangeListener`）：

| 回调 | 含义 | 引擎响应 |
|------|------|---------|
| `AUDIOFOCUS_GAIN` | 重新获得焦点 | 恢复播放（恢复原音量） |
| `AUDIOFOCUS_LOSS` | 永久失去（用户切到其它媒体） | 停止播放，需用户重新触发 |
| `AUDIOFOCUS_LOSS_TRANSIENT` | 暂时失去（来电/提示音） | 暂停，稍后自动恢复 |
| `AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK` | 暂时失去但可压低音量（导航播报） | **Ducking**（结合 P2-1 AudioBus::Duck） |

### 3.2 策略建议（游戏）

- **请求时机**：进入游戏主场景/开始播放 BGM 时请求焦点；退到后台或用户主动停止时放弃。
- **响应分级**：
  - `LOSS_TRANSIENT_CAN_DUCK` → 用 `AudioBus::Duck()` 压低音乐音量（不必停播）。
  - `LOSS_TRANSIENT` → 暂停播放，`GAIN` 时恢复。
  - `LOSS` → 停止，重置为"待用户触发"状态。
- 现代 Android（API 26+）使用 `AudioFocusRequest`（`AUDIOFOCUS_GAIN` + `AudioAttributes` 的 `USAGE_GAME`/`CONTENT_TYPE_MUSIC`）。

### 3.3 焦点状态映射

`AudioFocusState` 与 Android 回调的对应：

| AudioFocusState | Android 回调 |
|-----------------|-------------|
| `HasFocus` | `AUDIOFOCUS_GAIN` / 初始请求成功 |
| `LostTransient` | `LOSS_TRANSIENT` / `LOSS_TRANSIENT_CAN_DUCK` |
| `Lost` | `AUDIOFOCUS_LOSS` |

## 四、引擎集成

`AudioEngine` 应持有 `AudioSessionPolicy*`（构造时 `CreateSessionPolicy()` 创建，析构 `delete`），
并在 `Update()` 中驱动会话轮询，将焦点/静音状态转发到总线：

```cpp
// AudioEngine::Update(ct)
session->Update();

if(session->IsSilenced() || session->GetFocusState() == AudioFocusState::Lost)
    master->Duck(0.0f);                       // 静音/失焦：完全压低
else if(session->GetFocusState() == AudioFocusState::LostTransient)
    master->Duck(0.2f);                       // 暂时失焦：压低（duck）
else
    master->Unduck();                         // 正常：恢复
```

这样移动平台的会话状态直接作用于 P0-1/P2-1 已建成的总线树。

## 五、桌面/主机

`DesktopSessionPolicy`：恒 `HasFocus`、恒非静音，`RequestFocus`/`AbandonFocus` 仅维护状态（用于状态机测试与未来主机平台如 PS/Switch 的系统级音频中断预留接口）。

## 六、实现状态

| 平台 | 状态 |
|------|------|
| 桌面（Windows/macOS/Linux） | ✅ 完整实现（`DesktopSessionPolicy`） |
| iOS/iPadOS/tvOS | ⚠️ stub（需 Objective-C++ 桥接 `AVAudioSession`，条件编译于 `AudioSessionPolicy.cpp`） |
| Android | ⚠️ stub（需 JNI 桥接 `AudioManager`，条件编译于 `AudioSessionPolicy.cpp`） |

移动平台 stub 已随条件编译就位，接入对应 SDK 后填充 `iOSSessionPolicy`/`AndroidSessionPolicy` 的 TODO 即可，
无需改动接口与引擎侧代码。
