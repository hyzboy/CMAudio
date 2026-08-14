# CMAudio

CMAudio 是 [ULRE 游戏引擎](https://github.com/) 的音频子系统模块（CM 系列子模块之一，作者 hyzboy）。
它基于 OpenAL Soft 提供完整的游戏音频能力：从底层的音频资源加载、总线混音，到上层的 3D 空间音频、
数据驱动声音事件、动态音乐，以及一整套纯 CPU 的数字信号处理（DSP）工具链。

> 命名空间 `hgl::audio`，C++20，编译为静态库 `CMAudio`。

## 特性

- **音频引擎中枢**：`AudioEngine` 统一驱动总线树、资源管理、空间音频世界的每帧更新。
- **总线树混音**：`AudioBus` 树形总线（Master → Music/SFX/Ambient/UI），支持增益、静音、Ducking 与侧链压缩。
- **资源管理**：`AudioAssetManager` 缓存去重 + 引用计数 + 后台异步解码线程。
- **3D 空间音频**：`SpatialAudioWorld` 距离衰减、多普勒、锥形方向、频率相关衰减、场景低通、混响预设。
- **数据驱动事件**：`SoundEventManager` 用 TOML 配置声音事件（文件变体 + 参数随机化 + 分组）。
- **动态音乐**：`DynamicMusic` 多层 stem 播放 + 游戏状态驱动的 crossfade。
- **录音**：`AudioCapture` 封装 OpenAL 录音（语音聊天 / 语音指令）。
- **完整 DSP 工具链**（纯 CPU，无需 OpenAL EFX）：
  - 滤波：`BiquadFilter`（RBJ 8 类型）、`ParametricEQ`（N 段级联）、`AudioEQ`（缓冲级兜底）
  - 动态：`Compressor`（压缩/限制/侧链）
  - 响度：`LoudnessNormalizer`（EBU R128 LUFS 测量 + 归一化 + 峰值限制）
  - 时域：`TimeEffects`（DelayLine / Echo / Chorus / Flanger）
  - 分析：`AudioAnalysis`（FFT / RMS / 频谱 / onset 检测）
  - 变调与抖动：libsamplerate 高质量重采样 + mt19937 TPDF 抖动
- **重采样与离线混音**：`AudioResampler`（libsamplerate）、`AudioMixer`（多轨离线混音 + EQ + 软削波 + 抖动）、`AudioMixerScene`（TOML 场景混音）。
- **MIDI 播放**：`MIDIPlayer` / `MIDIOrchestraPlayer`（多通道控制、声像、独奏、管弦乐 3D 布局）。
- **移动平台会话策略**：`AudioSessionPolicy`（iOS 静音开关 / Android Audio Focus 抽象）。
- **插件系统**：3 个解码插件（WAV / Vorbis / Opus）+ 6 个 MIDI 合成器插件（FluidSynth / Timidity / TinySoundFont / WildMIDI / ADLMIDI / OPNMIDI）。

## 目录结构

```
CMAudio/
├── inc/hgl/audio/        # 公共头文件（41 个）
├── inc/hgl/al/           # OpenAL 绑定头
├── src/                  # 实现（39 个 .cpp）
├── Plug-Ins/             # 插件（解码器 + MIDI 合成器）
├── examples/             # 示例与测试（26 个）
├── doc/                  # 技术手册（Hugo 站点格式）
├── third_party/          # 第三方依赖
└── CMakeLists.txt
```

## 依赖

- **CMCore**（同系列基础库：类型、线程、容器、日志、数学）
- **OpenAL Soft**（`vcpkg install openal-soft:x64-windows`）
- **libsamplerate**（内置于 `src/libsamplerate/`）
- **libogg**（内置于 `Plug-Ins/libogg/`）

## 构建

CMAudio 作为 ULRE 的子模块，随 ULRE 一起构建；也可独立构建：

```bash
# 以 ULRE 集成方式（推荐）
cd D:/ULRE
cmake -S . -B build
cmake --build build --target CMAudio --config Debug

# 运行示例（测试二进制在 build/out/Windows_64_Debug/）
./build/out/Windows_64_Debug/engine_update_test.exe
```

> **运行 OpenAL 路径需要运行库**：把 `OpenAL32.dll` 与 `fmt.dll` 放到运行目录
> （`build/out/Windows_64_Debug/`）；无音频设备时用 null 后端测试。

## 快速开始

```cpp
#include <hgl/audio/AudioEngine.h>
#include <hgl/audio/AudioManager.h>
using namespace hgl::audio;

// 1. 创建引擎（自动建立 Master → Music/SFX/Ambient/UI 总线树）
AudioEngine engine;

// 2. 简单音效：AudioManager 维护一个音源池
AudioManager sfx(8);                    // 8 个并发音效源
sfx.SetBus(engine.GetSFX());            // 挂到 SFX 总线
sfx.Play(OS_TEXT("shot.wav"), 0.8f);    // 播放音效

// 3. 每帧驱动（资源上传 + 空间音频刷新）
while(running) {
    engine.Update(now);
}
```

完整示例见 [doc/getting-started.md](doc/getting-started.md)。

## 技术手册

详细 API 文档位于 `doc/` 目录，按 Hugo 站点格式编写：

| 手册 | 内容 |
|---|---|
| [核心引擎](doc/core-engine.md) | AudioEngine / AudioManager / AudioPlayer / AudioSource / AudioListener |
| [音频总线](doc/audio-bus.md) | AudioBus 树、增益、Ducking、侧链压缩 |
| [资源管理](doc/asset-management.md) | AudioAssetManager / AudioBuffer / 异步加载 |
| [声音事件与动态音乐](doc/sound-events.md) | SoundEventManager / DynamicMusic |
| [录音](doc/audio-capture.md) | AudioCapture |
| [空间音频](doc/spatial-audio.md) | SpatialAudioWorld / 方向性 / 混响 / 频率衰减 |
| [DSP 滤波](doc/dsp-filters.md) | BiquadFilter / ParametricEQ / AudioEQ |
| [DSP 动态与响度](doc/dsp-dynamics.md) | Compressor / LoudnessNormalizer / 侧链 |
| [DSP 时域效果](doc/dsp-time-effects.md) | DelayLine / Echo / Chorus / Flanger |
| [频谱分析](doc/dsp-analysis.md) | FFT / RMS / 频谱 / onset |
| [重采样与混音](doc/resample-mix.md) | AudioResampler / AudioMixer / AudioMixerScene |
| [MIDI 播放](doc/midi.md) | MIDIPlayer / MIDIOrchestraPlayer |
| [插件系统](doc/plugins.md) | 解码插件 / MIDI 合成器插件 |
| [会话策略](doc/session-policy.md) | AudioSessionPolicy（移动平台） |

## 示例程序

`examples/` 下每个测试都是可独立运行的程序（多数纯内存合成，无需外部音频文件）：

- 引擎/总线/资源：`engine_update_test`、`bus_tree_test`、`bus_ducking_test`、`sidechain_duck_test`、`asset_manager_test`、`async_load_test`
- 事件/音乐：`sound_event_test`、`dynamic_music_test`
- 录音：`audio_capture_test`
- DSP：`biquad_test`、`param_eq_test`、`eq_mixer_test`、`audio_eq_test`、`compressor_test`、`loudness_test`、`loudness_limiter_test`、`time_effect_test`、`audio_analysis_test`、`pitch_shift_test`
- 混音：`mixer_basic_test`、`scene_city_test`、`scene_swarm_test`、`wav_resample`
- 会话：`session_policy_test`

详见 [examples/README.md](examples/README.md)。
