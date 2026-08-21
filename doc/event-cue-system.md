---
title: "EVENT/CUE 机制设计"
linkTitle: "EVENT/CUE 机制"
weight: 15
date: 2026-08-17
description: "音频事件驱动架构设计：EVENT 指令协议、CUE 配置格式、传输层抽象、引擎线程化与三种部署形态"
draft: false
---

# EVENT/CUE 机制设计

本文档是 CMAudio 结构性重置（事件驱动架构）的**设计稿**：调用方完全通过 EVENT 指令控制播放，
音频引擎在独立线程（或独立进程）运行，与主程序隔离。核心思想对标行业标准
（Wwise Event / FMOD EventInstance）。

> 状态：**设计阶段**（T0）。`SoundEventConfig`（数据驱动事件）已存在可作为 CUE 定义基础；
> 事件指令层、传输层、引擎线程化为新增设计，尚未实现。

## 1. 核心概念：EVENT 与 CUE 是两层

```
调用方发的是 EVENT（指令：我要发生什么事）
音频引擎查的是 CUE （定义：这件事该怎么发声）

        "开枪！"                     "哦，开枪=从3个枪声变体里随机选一个，
        ──────►  EVENT  ──────►     音量±20%、音高±5%、挂SFX总线、带距离衰减、优先级8"
                                              ▲
                                              │ 按名查表
                                        CUE 定义表（TOML 配置驱动）
```

| | EVENT（事件） | CUE（提示/定义） |
|---|---|---|
| 本质 | **指令**，调用方发出的动作 | **数据**，描述"这个动作该怎么响应" |
| 内容 | `EventType + CueID + 参数` | 播放什么文件、多大音量、什么总线、是否循环、RTPC 映射 |
| 谁产生 | 游戏逻辑（每帧可发任意数量） | 音频策划（TOML 配置，不写代码） |
| 生命周期 | 瞬时（入队即完成发送） | 持久（引擎启动加载，常驻） |
| 类比 | 函数调用 | 函数体 |

**原则：调用方永远只说"发生了什么"，从不描述"怎么发声"。** 播放细节全部在 CUE 定义里，
策划改配置零代码变动。

## 2. 事件指令协议（跨边界安全）

跨线程/进程边界禁止裸指针，事件必须是**纯值（POD）结构**，二进制序列化。

### 2.1 指令类型

| 枚举 | 含义 | 参数 |
|---|---|---|
| `Play` | 触发一个 Cue | `cue_id`（目标 Cue 名哈希），可选 `pos[3]`（3D 位置） |
| `Stop` | 停止实例 | `instance_id`（0=停止该 Cue 的全部实例） |
| `SetParam` | 实时参数（RTPC） | `param_id` + `value`（如 "rpm"=4500） |
| `SetBusVolume` | 总线音量 | `bus_id` + `gain` |
| `SetBusMute` | 总线静音 | `bus_id` + `mute` |
| `LoadCue` | 加载 Cue 包（按需） | `pack_id`（关卡/场景音效包） |
| `UnloadCue` | 卸载 Cue 包 | `pack_id` |
| `Snapshot` | 切换混音快照 | `snapshot_id`（如"进菜单压低环境声"） |
| `PauseAll` / `ResumeAll` | 全局暂停/恢复 | 无 |

### 2.2 事件结构（POD，固定布局）

```cpp
// 跨边界安全：无指针、无虚表、固定大小
struct AudioEvent
{
    uint32  type;           // AudioEventType
    uint32  cue_id;         // Cue 名 FNV-1a 哈希（引擎侧查表；查不到记日志丢弃）
    uint32  instance_id;    // 目标实例（Play=0 表示新建；Stop/SetParam 定位实例）
    float   params[8];      // 定长参数块：pos[3] / rtpc value / gain / pan / ...
    uint32  seq;            // 发送端序号（调试/对账）
};
static_assert(sizeof(AudioEvent) == 48, "跨边界事件必须固定大小");
```

- **Cue 名用哈希而非字符串**：定长、可 memcpy、跨进程安全；引擎侧维护 `哈希→Cue 定义` 表
- 字符串类参数（调试用事件名）走**边带通道**（见 6.4），不在热路径
- `seq` 用于调试：引擎侧可回显"已处理到 seq N"定位丢事件

### 2.3 状态回传（引擎 → 调用方，反向通道）

| 回传类型 | 内容 |
|---|---|
| `PlayStarted` | 实例已创建（回 `instance_id`） |
| `PlayFinished` | 实例自然结束 |
| `Stopped` | 实例被 Stop / 被优先级抢占 |
| `LoadComplete` | Cue 包加载完成 |
| `Error` | 无效 Cue / 资源缺失 / 设备失败 |

```cpp
struct AudioEventResult
{
    uint32  type;           // 回传类型
    uint32  instance_id;    // 相关实例（0=全局）
    uint32  error_code;     // 0=OK
    uint32  seq;            // 对应事件的 seq（对账）
};
```

回传通道与发送通道同构（反向的传输层），调用方**轮询或回调**取结果。

## 3. CUE 定义格式（TOML）

在现有 `sound_events.toml` 基础上扩展（现 `SoundEventConfig` 的字段全部保留为子集）。

```toml
# ===== 基础 Cue（对应现有 SoundEventConfig）=====
[cues.ui_click]
files = ["ui/click_01.ogg", "ui/click_02.ogg"]   # 变体：播放时随机选一
bus = "SFX"
gain = { min = 0.8, max = 1.2 }       # 音量随机化 ±20%
pitch = { min = -0.05, max = 0.05 }   # 音高随机化 ±5%
priority = 8
loop = false

# ===== 3D Cue =====
[cues.explosion]
files = ["sfx/boom.ogg"]
bus = "SFX"
reference_distance = 1.0
max_distance = 60.0
rolloff_factor = 1.0
loop = false

# ===== 循环 + 停止 =====
[cues.engine_idle]
files = ["vehicle/idle.ogg"]
bus = "SFX"
loop = true
priority = 5

# ===== 复合 Cue（一个事件触发多个声音）=====
[cues.combo_hit]
children = ["hit_body", "hit_voice", "hit_spark"]   # 子 Cue 并行触发
[children_gain.hit_voice]                            # 可对子 Cue 单独调音量
min = 0.5
max = 0.5

# ===== 随机序列 Cue（脚步左右交替）=====
[cues.step_cycle]
sequence = ["step_l.ogg", "step_r.ogg"]   # 严格按序轮播（区别于 files 随机）
loop = true

# ===== RTPC：实时参数映射 =====
[cues.engine_rpm]
files = ["vehicle/engine.ogg"]
bus = "SFX"
loop = true

# 游戏发 SetParam("rpm", v)，v∈[0,8000] 线性映射到 pitch∈[0.5,2.0]
[[rtpc]]
cue = "engine_rpm"
param = "rpm"
min = 0.0
max = 8000.0
target = "pitch"
min_value = 0.5
max_value = 2.0

# 同一参数可同时映射多个目标（音调 + 低通滤波）
[[rtpc]]
cue = "engine_rpm"
param = "rpm"
min = 0.0
max = 8000.0
target = "lowpass"
min_value = 200.0
max_value = 8000.0

# ===== 快照（混音状态切换）=====
[snapshots.in_menu]
# 进菜单：音乐 -6dB、环境声 -12dB、UI 正常
[snapshots.in_menu.bus_gain]
Music = -6.0
Ambient = -12.0
UI = 0.0
```

### 3.1 Cue 字段清单（= 现有 SoundEventConfig 超集）

| 字段 | 类型 | 说明 |
|---|---|---|
| `files` | string[] | 变体列表（随机选一）；与 `sequence`/`children` 三选一 |
| `sequence` | string[] | 严格按序轮播 |
| `children` | string[] | 复合 Cue：并行触发子 Cue |
| `bus` | enum | Master/Music/SFX/Ambient/UI |
| `gain` | {min,max} | 音量随机化 |
| `pitch` | {min,max} | 音高随机化（半音） |
| `priority` | float | 调度优先级（音源不足时抢占低优先级） |
| `loop` | bool | 循环 |
| `reference_distance` / `max_distance` / `rolloff_factor` | float | 3D 衰减 |
| `rtpc` | table[] | 实时参数映射（见 3.2） |

### 3.2 RTPC 映射

```
游戏逻辑                   音频引擎内部
SetParam("rpm", 4500) ──►  查 Cue.rtpc 表：param=="rpm"
                            ──► 4500 ∈ [0,8000] → 归一化 0.5625
                            ──► 线性映射到 target="pitch" ∈ [0.5,2.0] → 1.34
                            ──► 应用到该 Cue 的所有活跃实例
```

**游戏代码完全不知道音频内部怎么做**——这是 EVENT 机制的核心解耦价值。

## 4. 传输层抽象（三种部署形态共用）

```
                    ┌─────────────────────────────┐
                    │  EventTransport（纯虚接口）   │
                    │  - Send(const AudioEvent&)  │
                    │  - PollResult(AudioEventResult&) │
                    └──────────────┬──────────────┘
              ┌────────────────────┼────────────────────┐
              ▼                    ▼                    ▼
    SameProcessQueue      DLLExportTransport      IPCTransport
    （静态库模式）          （DLL/SO 模式）          （独占进程模式）
```

| 实现 | 通道 | 说明 |
|---|---|---|
| `SameProcessQueue` | 无锁 SPSC/MPSC 环形队列（共享内存） | 同进程零拷贝，性能最高 |
| `DLLExportTransport` | 同进程队列 + 导出 C API 包装 | DLL 内同样共享内存，C 接口保 ABI |
| `IPCTransport` | 共享内存队列 + 命名管道（事件/唤醒） | 跨进程；共享内存传数据、管道传信号量 |

**统一语义**（三种实现必须一致）：
- `Send` 永不阻塞（队列满则丢 + 计数告警，音频不能卡游戏）
- `PollResult` 非阻塞轮询
- 事件消费在**音频帧边界批量处理**（每 tick 清空队列），保证帧内一致性

## 5. 引擎线程化（隔离的核心）

### 5.1 现状 → 目标

```
现状：主线程每帧调用 AudioEngine::Update()
目标：引擎内部独立线程，主循环 = 事件消费 + Update()
```

```cpp
class AudioEngineThread          // 新增：引擎线程壳
{
    EventTransport *transport;   // 事件入口（三形态之一）
    AudioEngine     engine;      // 现有引擎（内部含总线/资源/DSP/事件表）
    hgl::Thread     thread;
    atom<bool>      running;

public:
    bool Start();                // 起线程：初始化 OpenAL context + 加载 Cue 表
    void Stop();                 // 停线程（排空队列、释放资源、关设备）

private:
    void MainLoop();             // 线程体：
                                 // 1. 批量消费事件队列（Send 的事件）
                                 // 2. engine.Update(now)（驱动总线/资源/空间音频）
                                 // 3. 处理回传队列（PlayStarted/Finished...）
                                 // 4. SleepSecond(帧间隔 ~10ms)
};
```

### 5.2 线程隔离规则

- **OpenAL context 只在音频线程创建/使用**——主线程绝不触碰（现 `AudioAssetManager`
  的"工作线程解码、主线程上传"模式需要调整：解码与上传都在音频线程内）
- 调用方线程只碰 `EventTransport`（无锁队列），无任何共享状态
- 状态查询（`GetBusVolume` 等）走"快照"模式：引擎线程每帧发布只读快照，
  调用方轮询读取（原子指针换发），不实时调引擎内部

### 5.3 同步点（测试/工具用）

```cpp
bool WaitIdle(uint timeout_ms);   // 等待队列排空 + 本帧处理完成（测试断言用）
```

## 6. 客户端 SDK（调用方视角）

三种形态统一暴露同一套 C API（静态库模式额外提供 C++ inline 头走零拷贝捷径）：

```c
// 初始化/销毁（三形态差异只在这里）
//   静态库: hgl_audio_init(&queue_transport_cfg)
//   DLL  : hgl_audio_load("CMAudioClient.dll", &cfg)
//   进程 : hgl_audio_connect("cm_audio_service", &cfg)
HGL_AUDIO_API bool hgl_audio_init(const AudioClientConfig *cfg);
HGL_AUDIO_API void hgl_audio_shutdown(void);

// 事件发送（核心，全部异步）
HGL_AUDIO_API void hgl_audio_play(const char *cue_name);                 // EVT_Play
HGL_AUDIO_API void hgl_audio_play_at(const char *cue_name, float x,float y,float z);
HGL_AUDIO_API void hgl_audio_stop(uint32 instance_id);                  // EVT_Stop
HGL_AUDIO_API void hgl_audio_set_param(uint32 instance_id,const char *param,float value); // RTPC
HGL_AUDIO_API void hgl_audio_set_bus_volume(int bus,float gain);        // EVT_SetBusVolume
HGL_AUDIO_API void hgl_audio_load_cue_pack(const char *pack);           // EVT_LoadCue
HGL_AUDIO_API void hgl_audio_snapshot(const char *snapshot);            // EVT_Snapshot

// 状态查询（快照轮询，非阻塞）
HGL_AUDIO_API bool hgl_audio_poll_result(AudioEventResult *out);        // 回传轮询
HGL_AUDIO_API const AudioStateSnapshot *hgl_audio_get_state(void);      // 只读快照
```

## 7. 与现有代码的关系

| 现有组件 | 重置后的角色 | 改动 |
|---|---|---|
| `SoundEventManager` | Cue 表容器（升级：哈希索引 + 包管理 + RTPC 表） | 中 |
| `SoundEventConfig` | Cue 定义结构（字段超集：sequence/children/rtpc） | 中 |
| `AudioEngine::Update()` | 引擎线程主循环体（原样复用） | 小 |
| `AudioAssetManager` | 引擎线程内自主加载（解码+上传都在音频线程） | 中 |
| `AudioPlayer`/`AudioSource` | 引擎内部播放原语（对外不再暴露） | 大（封装） |
| `VoiceCall`/DSP 链 | 引擎内部能力（事件可触发变声/通话） | 小 |
| 30 个测试 | 改为事件驱动 + `WaitIdle` 同步 | 大（重写） |

> ULRE 引擎侧对 CMAudio 零调用，破坏性重构无外部迁移成本。

## 8. 三种部署形态对比（回顾）

| | 静态库 LINK | DLL/SO | 独占进程 |
|---|---|---|---|
| 传输层 | 同进程无锁队列 | 同进程队列 + C 导出 | 共享内存 + 管道 |
| ABI | 无 | C 接口保稳定 | 协议二进制（最强隔离） |
| 崩溃影响 | 同进程崩溃 | 同进程崩溃 | **音频崩溃不拖垮主程序，可自动重启** |
| 性能 | 最高（零拷贝） | 高 | 中（memcpy + 唤醒开销） |
| 典型场景 | 游戏发布版 | 编辑器/工具/热更新 | 音频服务/多客户端共享/强隔离 |

## 9. 实施路线（每步可构建验证）

- **T1** 事件协议层：`AudioEvent`/`AudioEventResult` 结构 + 序列化 + 单测
- **T2** Cue 配置扩展：TOML 新字段（sequence/children/rtpc/snapshot）+ 解析 + 单测
- **T3** 传输层：`EventTransport` 抽象 + `SameProcessQueue`（无锁环形队列）+ 单测
- **T4** 引擎线程化：`AudioEngineThread` + 事件消费 + `WaitIdle`（静态库模式全链落地）
- **T5** 现有 API → 事件指令映射（Play/Stop/SetParam/总线），测试改事件驱动
- **T6** DLL 模式：导出 C API + 客户端 SDK 头
- **T7** 进程模式：`IPCTransport`（共享内存 + 命名管道）+ 生命周期管理 + 崩溃恢复
- **T8** 文档（三形态部署手册）+ 全量回归

## 10. 关键设计决策摘要

1. **事件 = 纯值 POD（48 字节定长）**，Cue 名用哈希——跨线程/进程边界安全
2. **Send 永不阻塞**——音频不能卡游戏；满队列丢弃 + 计数告警
3. **音频帧边界批量消费**——事件在 `Update()` 前一次性入队执行，帧内一致
4. **回传与发送同构**——反向传输层，轮询/回调均可
5. **OpenAL context 完全归属音频线程**——隔离的物理保证
6. **状态查询走快照**——调用方轮询只读快照，不实时触碰引擎内部
7. **三种形态只换传输层**——引擎核心、Cue 表、事件解析完全复用
