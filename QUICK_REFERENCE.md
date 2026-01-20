# AudioPlayer 快速参考指南 / Quick Reference Guide

## 快速开始 / Quick Start

### 基本使用 / Basic Usage

```cpp
// 1. 创建播放器 / Create player
AudioPlayer player;

// 2. 加载音频文件 / Load audio file
player.Load("background_music.ogg");

// 3. 开始播放 / Start playback
player.Play(true);  // true = 循环播放 / loop playback

// 4. 控制播放 / Control playback
player.Pause();     // 暂停 / Pause
player.Resume();    // 继续 / Resume
player.Stop();      // 停止 / Stop
```

### 音量控制 / Volume Control

```cpp
// 设置音量 (0.0 - 1.0) / Set volume (0.0 - 1.0)
player.SetGain(0.8f);

// 获取当前音量 / Get current volume
float volume = player.GetGain();

// 自动音量渐变 / Auto volume fade
player.AutoGain(0.5f, 3.0, GetTimeSec());  // 3秒内渐变到0.5
```

### 淡入淡出效果 / Fade In/Out Effects

```cpp
// 设置淡入淡出时间 / Set fade times
player.SetFadeTime(2.0, 3.0);  // 2秒淡入，3秒淡出
player.Play(true);
```

### 3D空间音频 / 3D Spatial Audio

```cpp
// 设置音源位置 / Set source position
player.SetPosition(Vector3f(10.0f, 0.0f, 5.0f));

// 设置音源方向 / Set source direction
player.SetDirection(Vector3f(0.0f, 0.0f, -1.0f));

// 设置距离参数 / Set distance parameters
player.SetDistance(1.0f, 100.0f);  // 参考距离，最大距离

// 设置衰减因子 / Set rolloff factor
player.SetRolloffFactor(1.0f);
```

### 音调控制 / Pitch Control

```cpp
// 改变播放速度和音调 / Change playback speed and pitch
player.SetPitch(1.2f);  // 1.2倍速，音调升高
player.SetPitch(0.8f);  // 0.8倍速，音调降低
player.SetPitch(1.0f);  // 正常速度
```

## 常见模式 / Common Patterns

### 背景音乐循环播放 / Background Music Loop

```cpp
AudioPlayer bgm;
bgm.Load("bgm.ogg");
bgm.SetFadeTime(2.0, 2.0);
bgm.SetGain(0.7f);
bgm.Play(true);  // 循环播放
```

### 场景转换音乐淡出 / Scene Transition Fade Out

```cpp
// 3秒内淡出到静音 / Fade to silence in 3 seconds
bgm.AutoGain(0.0f, 3.0, GetTimeSec());
```

### 3D环境音效 / 3D Environmental Sound

```cpp
AudioPlayer ambient("waterfall.ogg");
ambient.SetPosition(Vector3f(20, 5, 10));
ambient.SetDistance(5.0f, 50.0f);
ambient.SetGain(0.5f);
ambient.Play(true);
```

### 方向性音效 / Directional Sound

```cpp
AudioPlayer speaker;
speaker.Load("announcement.ogg");
speaker.SetPosition(Vector3f(0, 2, 0));
speaker.SetDirection(Vector3f(1, 0, 0));  // 向右发声
speaker.SetConeAngle(ConeAngle(30.0f, 45.0f));  // 内角30度，外角45度
speaker.Play(false);
```

## API 参考 / API Reference

### 构造函数 / Constructors

```cpp
AudioPlayer();
AudioPlayer(const os_char *filename, AudioFileType=AudioFileType::None);
AudioPlayer(io::InputStream *stream, int size, AudioFileType);
```

### 加载方法 / Loading Methods

```cpp
bool Load(const os_char *filename, AudioFileType=AudioFileType::None);
bool Load(io::InputStream *stream, int size, AudioFileType);
void Clear();  // 清除当前音频数据
```

### 播放控制 / Playback Control

```cpp
void Play(bool loop=true);     // 开始播放
void Stop();                   // 停止播放
void Pause();                  // 暂停播放
void Resume();                 // 继续播放
bool IsLoop();                 // 是否循环
void SetLoop(bool loop);       // 设置循环
```

### 状态查询 / State Query

```cpp
PlayState GetPlayState() const;        // 获取播放状态
int GetSourceState() const;            // 获取音源状态
double GetTotalTime() const;           // 获取总时长
PreciseTime GetPlayTime();             // 获取已播放时间
```

### 音量和音调 / Volume and Pitch

```cpp
float GetGain() const;                 // 获取音量
void SetGain(float gain);              // 设置音量 (0.0-1.0)
float GetMinGain() const;              // 获取最小音量
float GetMaxGain() const;              // 获取最大音量
float GetPitch() const;                // 获取音调
void SetPitch(float pitch);            // 设置音调 (0.5-2.0通常)
```

### 3D 位置和方向 / 3D Position and Direction

```cpp
const Vector3f& GetPosition() const;
void SetPosition(const Vector3f &pos);

const Vector3f& GetVelocity() const;
void SetVelocity(const Vector3f &vel);

const Vector3f& GetDirection() const;
void SetDirection(const Vector3f &dir);
```

### 距离衰减 / Distance Attenuation

```cpp
void GetDistance(float &ref_distance, float &max_distance) const;
void SetDistance(const float &ref_distance, const float &max_distance);

float GetRolloffFactor() const;
void SetRolloffFactor(float factor);
```

### 锥形音效 / Cone Effects

```cpp
const ConeAngle& GetConeAngle() const;
void SetConeAngle(const ConeAngle &angle);

float GetConeGain() const;
void SetConeGain(float gain);
```

### 高级效果 / Advanced Effects

```cpp
void SetFadeTime(PreciseTime fade_in, PreciseTime fade_out);
void AutoGain(float target_gain, PreciseTime adjust_time, const PreciseTime cur_time);
```

## 播放状态 / Play States

```cpp
enum class PlayState {
    None,    // 未播放 / Not playing
    Play,    // 正在播放 / Playing
    Pause,   // 已暂停 / Paused
    Exit     // 正在退出 / Exiting
};
```

## 支持的音频格式 / Supported Audio Formats

通过插件系统支持 / Supported via plugin system:
- **Opus** (.opus)
- **OGG Vorbis** (.ogg)
- 其他格式可通过插件扩展 / Other formats via plugins

## 性能提示 / Performance Tips

### ✅ 推荐做法 / Do's

1. **长音频使用 AudioPlayer** / Use AudioPlayer for long audio
   - 背景音乐 / Background music
   - 环境音效 / Ambient sounds
   - 对话音频 / Dialog audio

2. **预加载音频** / Preload audio
   ```cpp
   player.Load("music.ogg");  // 在需要之前加载
   // ... later ...
   player.Play(true);
   ```

3. **合理设置缓冲** / Reasonable buffering
   - 默认1/10秒缓冲已优化 / Default 1/10s buffer is optimized

4. **使用淡入淡出** / Use fade effects
   ```cpp
   player.SetFadeTime(1.0, 1.0);  // 避免突然开始/停止
   ```

### ❌ 避免做法 / Don'ts

1. **不要用于短音效** / Don't use for short sound effects
   - 每个实例占用一个线程 / Each instance uses one thread
   - 短音效用 AudioSource + AudioBuffer

2. **播放时不要重新加载** / Don't reload while playing
   ```cpp
   player.Stop();        // 先停止
   player.Load("new.ogg"); // 再加载
   ```

3. **不要频繁创建/销毁** / Don't create/destroy frequently
   - 重用实例 / Reuse instances
   - 使用对象池 / Use object pool

4. **避免播放时销毁** / Don't destroy while playing
   ```cpp
   player.Stop();  // 确保停止后再销毁
   // ~AudioPlayer()
   ```

## 常见问题 / FAQ

### Q: 如何实现无缝循环？
A: 使用 `Play(true)` 启用循环。注意：循环通过重启解码器实现，可能有极短暂的间隙。

### Q: How to achieve seamless looping?
A: Use `Play(true)` to enable looping. Note: Looping is implemented by restarting decoder, may have brief gap.

### Q: 能否同时播放多个音频？
A: 每个 AudioPlayer 实例只能播放一个音频。需要同时播放多个，创建多个实例。

### Q: Can I play multiple audio files simultaneously?
A: Each AudioPlayer instance plays one audio file. Create multiple instances for simultaneous playback.

### Q: 内存占用如何？
A: AudioPlayer 将整个压缩音频文件加载到内存。对于大文件，注意内存使用。

### Q: What about memory usage?
A: AudioPlayer loads entire compressed audio file to memory. Be mindful with large files.

### Q: 支持流式加载吗？
A: 当前不支持从磁盘流式加载，但支持从内存流式解码到 OpenAL。

### Q: Does it support streaming from disk?
A: Currently doesn't support streaming from disk, but supports streaming decode from memory to OpenAL.

### Q: 线程安全吗？
A: 是的，关键操作使用互斥锁保护。但避免在播放时调用 Load() 或销毁对象。

### Q: Is it thread-safe?
A: Yes, critical operations are protected with mutex. But avoid calling Load() or destroying object during playback.

## 调试技巧 / Debugging Tips

### 检查播放状态 / Check Playback State

```cpp
PlayState state = player.GetPlayState();
switch(state) {
    case PlayState::None:  LOG("未播放"); break;
    case PlayState::Play:  LOG("正在播放"); break;
    case PlayState::Pause: LOG("已暂停"); break;
    case PlayState::Exit:  LOG("正在退出"); break;
}
```

### 检查音源状态 / Check Source State

```cpp
int state = player.GetSourceState();
if(state == AL_PLAYING) LOG("OpenAL正在播放");
if(state == AL_STOPPED) LOG("OpenAL已停止");
if(state == AL_PAUSED)  LOG("OpenAL已暂停");
```

### 监控播放进度 / Monitor Playback Progress

```cpp
double current = player.GetPlayTime();
double total = player.GetTotalTime();
double progress = (current / total) * 100.0;
LOG("播放进度: %.1f%%", progress);
```

## 更多资源 / More Resources

- **详细分析**: AudioPlayer_Analysis.md (中文)
- **Detailed Analysis**: AudioPlayer_Analysis_EN.md (English)
- **架构图表**: AudioPlayer_Diagrams.md
- **总结摘要**: ANALYSIS_SUMMARY.md

---

**注意 / Note**: 本指南基于代码分析生成，实际使用时请参考项目文档和示例代码。
This guide is generated from code analysis. Please refer to project documentation and example code for actual usage.
