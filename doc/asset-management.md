---
title: "资源管理"
linkTitle: "资源管理"
weight: 30
date: 2026-08-15
description: "AudioAssetManager 缓存/引用计数/异步加载，AudioBuffer 缓冲区与格式"
draft: false
---

# 资源管理

资源管理由 `AudioAssetManager`（缓存与加载）与 `AudioBuffer`（音频数据）构成，
配合 `AudioFileType` 做格式识别。

## AudioFileType（文件格式）

```cpp
enum class AudioFileType { None, Wav, Vorbis, Opus, MIDI };

AudioFileType CheckAudioExtName(const os_char *ext_name);      // 扩展名 → 类型
AudioFileType CheckAudioFileType(const os_char *filename);    // 文件名 → 类型（None=自动识别）
```

加载时传 `AudioFileType::None` 表示按扩展名自动识别。

## AudioBuffer（音频数据缓冲区）

`AudioBuffer` 是已解码 PCM 的载体，绑定到 OpenAL buffer，可被任意 `AudioSource` 引用：

```cpp
class AudioBuffer
{
    uint GetIndex() const;    double GetTime() const;   // 时长（秒）
    uint GetSize() const;     uint GetFreq() const;     // 字节数 / 采样率
    bool IsLoaded() const;

    uint GetRefCount() const;  uint IncRef();  uint DecRef();   // 引用计数

    bool Load(const os_char *filename, AudioFileType = None);   // 从文件加载
    bool Load(void *data, int size, AudioFileType);             // 从内存加载
    bool Load(io::InputStream *stream, int size, AudioFileType);// 从流加载
    bool SetData(uint format, const void *data, uint size, uint freq);
    bool SetData(const AudioDataInfo &info, const void *data);

    ParametricEQ &GetEQ();   void ClearEQ();   // buffer 级 EQ（加载时一次性频率整形）
    void Clear();
};
```

> **buffer 级 EQ**：`AudioBuffer::Load/SetData` 在解码后、上传 OpenAL 前应用 `GetEQ()` 的频段
> （CPU 路径，EFX 缺失时的兜底）。这是**加载时一次性整形**，不是播放中实时 EQ。

## AudioAssetManager（缓存 + 引用计数 + 异步加载）

```cpp
class AudioAssetManager
{
    AudioBuffer *Acquire(const os_char *filename);      // 命中缓存 ref+1，否则同步加载(ref=1)
    bool Register(const os_char *name, AudioBuffer *);  // 手动登记（测试注入/自定义加载）
    void Release(AudioBuffer *);                        // ref-1，归零时卸载并移除
    void Release(const os_char *name);
    AudioBuffer *Find(const os_char *name) const;       // 只读查找（不改引用计数）
    bool Contains(const os_char *name) const;
    int  GetCount() const;
    void Clear();                                       // 清空全部缓存

    // 异步加载（P0-2）
    bool AcquireAsync(const os_char *filename);         // 提交后台解码任务
    int  Update();                                      // 主线程每帧上传已解码缓冲
    bool IsLoading() const;
    int  GetPendingCount() const;
};
```

**核心语义**：
- 同一文件（按 filename 字符串去重）只加载一次，多次 `Acquire` 共享同一 `AudioBuffer`。
- 引用计数归零自动卸载；`Clear()` 强制清空（无视引用计数）。
- 所有缓存操作在内部互斥锁保护下，线程安全。

```cpp
AudioAssetManager assets;

AudioBuffer *a = assets.Acquire(OS_TEXT("shot.wav"));   // 加载，ref=1
AudioBuffer *b = assets.Acquire(OS_TEXT("shot.wav"));   // 命中缓存，ref=2
assert(a == b);

assets.Release(a);                                      // ref=1
assets.Release(OS_TEXT("shot.wav"));                    // ref=0，自动卸载
assert(assets.Find(OS_TEXT("shot.wav")) == nullptr);
```

### 异步加载

异步路径把**解码（纯 CPU/IO）**放到后台线程，**上传（OpenAL 调用）**留在主线程 `Update()`，
避免 OpenAL 上下文被跨线程访问：

```cpp
assets.AcquireAsync(OS_TEXT("big_bgm.ogg"));    // 提交后台解码

while(assets.IsLoading()) {                      // 轮询（或在主循环 Update）
    assets.Update();                             // 上传完成的缓冲并登记缓存
}
AudioBuffer *buf = assets.Acquire(OS_TEXT("big_bgm.ogg"));  // 命中缓存，零成本
```

### 加载模式建议

```cpp
enum class AudioLoadMode { Full, Stream };

AudioLoadMode SuggestAudioLoadMode(int64 file_size, int64 full_load_threshold = 1MB);
```

启发式：小文件（默认 < 1MB）全量常驻（`AudioBuffer`，适合音效），
大文件流式（`AudioPlayer`，适合 BGM）。
