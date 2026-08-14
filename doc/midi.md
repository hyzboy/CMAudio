---
title: "MIDI 播放"
linkTitle: "MIDI 播放"
weight: 120
date: 2026-08-15
description: "MIDIPlayer 多通道控制、MIDIOrchestraPlayer 管弦乐 3D 布局、GM 乐器编号"
draft: false
---

# MIDI 播放

CMAudio 提供两个 MIDI 播放器，后端由 6 个 MIDI 合成器插件之一驱动（见 [插件系统](plugins.md)）。

## MIDIInstrument（GM 乐器编号）

```cpp
// General MIDI Level 1 乐器编号（enum class，类型安全）
// 0=Piano, 24=Guitar, 40=Violin, ... （详见 MIDIInstrument.h 完整列表）
```

## MIDIPlayer（专业 MIDI 播放器）

`MIDIPlayer` 提供对 MIDI 文件的完整控制：

```cpp
class MIDIPlayer
{
    bool SetSoundFont(const char *path);   // 设置音色库
    bool SetBank(int bank_id);             // 设置音色库编号
    bool SetBankFile(const char *path);
    bool SetSampleRate(int sample_rate);

    int  GetChannelCount() const;
    bool GetChannelInfo(int channel, MIDIChannelInfo *info) const;
    bool SetChannelProgram(int channel, int program);   // 更换乐器
    bool SetChannelVolume(int channel, int volume);     // 通道音量
    bool SetChannelPan(int channel, int pan);           // 声像
    bool SetChannelMute(int channel, bool mute);        // 静音
    bool SetChannelSolo(int channel, bool solo);        // 独奏
    bool ResetChannel(int channel);   bool ResetAllChannels();
    bool GetChannelActivity(int channel) const;         // 通道是否有音符活动

    int DecodeChannel(int channel, int16_t **buffer, int samples);   // 分离解码单通道
    int DecodeAllChannels(int16_t **buffers, int samples);           // 分离解码所有通道
};
```

```cpp
MIDIPlayer midi(OS_TEXT("song.mid"));
midi.SetSoundFont("soundfont.sf2");
midi.SetChannelProgram(0, 40);        // 通道 0 换小提琴
midi.SetChannelVolume(0, 100);
midi.SetChannelPan(0, 64);
midi.SetChannelMute(1, true);         // 静音通道 1
midi.SetChannelSolo(2, true);         // 独奏通道 2
```

## MIDIOrchestraPlayer（管弦乐 3D 布局）

`MIDIOrchestraPlayer` 在 MIDIPlayer 基础上，把各通道乐器摆到 3D 空间，模拟管弦乐队声场：

```cpp
class MIDIOrchestraPlayer
{
    bool Load(const os_char *filename);   bool Load(io::InputStream *, int size=-1);
    void Play(bool loop=false);  void Pause();  void Resume();  void Stop();  void Close();

    void SetSoundFont(const char *path);  void SetBank(int bank_id);
    void SetSampleRate(int sample_rate);

    void SetLayout(OrchestraLayout layout);              // 预定义乐队布局
    void SetOrchestraCenter(const Vector3f &center);     // 乐队中心
    void SetOrchestraScale(float scale);                 // 整体尺度
    void SetChannelPosition(int channel, const Vector3f &position);
    Vector3f GetChannelPosition(int channel) const;

    void SetChannelVolume(int channel, float volume);
    void SetChannelEnabled(int channel, bool enabled);
    void SetChannelProgram(int channel, int program);
    void MuteChannel(int channel, bool mute);
    void SoloChannel(int channel, bool solo);
    AudioSource *GetChannelSource(int channel);          // 取得通道对应的 AudioSource（3D 定位）

    void AutoGain(float start_gain, float gap, float end_gain);  // 各通道自动增益
};
```

```cpp
MIDIOrchestraPlayer orch;
orch.Load(OS_TEXT("symphony.mid"));
orch.SetSoundFont("orchestra.sf2");
orch.SetLayout(OrchestraLayout::Standard);
orch.SetOrchestraCenter({0, 0, 0});
orch.SetOrchestraScale(10.0f);
orch.SetBus(engine.GetMusic());
orch.Play(true);
```

> 通道的 3D 声像通过 `GetChannelSource(channel)` 得到的 `AudioSource` 定位实现，
> 与 `SpatialAudioWorld` / `AudioListener` 协同形成完整的管弦乐空间声场。
