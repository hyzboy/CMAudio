---
title: "音频总线"
linkTitle: "音频总线"
weight: 20
date: 2026-08-15
description: "AudioBus 树形总线、增益、静音、Ducking 与侧链压缩"
draft: false
---

# 音频总线（AudioBus）

`AudioBus` 构成一棵总线树（Master → Music/SFX/Ambient/UI → ...），
每个 `AudioSource` 挂载到某个总线，其有效增益沿父链累积：

```text
有效增益 = 父链有效增益 × 本节点增益 × 静音系数 × Duck 缩放
```

## 总线树

```cpp
class AudioBus
{
    AudioBus *GetParent() const;
    const AnsiString &GetName() const;

    void SetGain(float);     float GetGain() const;     // 本节点增益
    void SetMute(bool);      bool IsMute() const;
    float GetEffectiveGain() const;                     // 含父链/静音/Duck 的最终增益

    AudioBus *CreateChild(const char *name);            // 创建子总线
    void AttachSource(AudioSource *);                    // 挂接音源（由 SetBus 调用）
    void DetachSource(AudioSource *);
};
```

```cpp
AudioBus master;                       // 根
AudioBus *music = master.CreateChild("Music");
AudioBus *sfx   = master.CreateChild("SFX");
AudioBus *ui    = master.CreateChild("UI");
AudioBus *ui_warn = ui->CreateChild("Warning");   // 可继续嵌套

music->SetGain(0.8f);                  // 音乐总线整体 80%
ui_warn->SetMute(true);                // 静音警告子总线
```

`AudioEngine` 已为你建好标准四总线（Music/SFX/Ambient/UI），直接 `engine.GetMusic()` 等取用即可。

## Ducking（压低）

Ducking 用于"语音/重要音效压低背景音乐"：临时把某总线（及子树）增益平滑压低到目标值：

```cpp
void Duck(float target_scale, double duration=0.2, double now=0);  // 平滑压低
void Unduck(double duration=0.2, double now=0);                    // 恢复
float GetDuckScale() const;
bool IsDucked() const;
void Update(const double now);                                     // 每帧驱动平滑过渡
```

```cpp
// 播放语音时压低音乐
music->Duck(0.2, 0.3, now);    // 300ms 内压到 20%
// ...
music->Unduck(0.5, now);       // 语音结束，500ms 恢复
```

`Update()` 需要每帧调用以驱动 `GainRamp` 平滑过渡（`AudioEngine::Update()` 会递归驱动）。

## 侧链压缩 Duck（P3 延伸）

静态 Duck 用固定目标值；侧链压缩 Duck 则用**侧链信号电平**动态驱动一个压缩器，
替代固定 GainRamp，更贴合真实混音（如"枪声按响度动态压低音乐"）：

```cpp
void SetSidechainDuck(float sample_rate,
                      float threshold_db = -20.0f,   // 阈值
                      float ratio = 4.0f,            // 压缩比
                      float attack_sec = 0.01f,
                      float release_sec = 0.1f);
void UpdateSidechainDuck(float sidechain_level);     // 每帧用侧链电平驱动（0=无，1=满幅）
void DisableSidechainDuck();                          // 关闭，恢复 duck_scale=1.0
bool IsSidechainDuckEnabled() const;
```

```cpp
music->SetSidechainDuck(48000, -20.0f, 4.0f, 0.01f, 0.1f);

// 每帧：用语音（侧链）的当前电平驱动
float voice_level = MeasureVoiceLevel();     // 0.0 ~ 1.0
music->UpdateSidechainDuck(voice_level);
```

- 侧链电平越高，压低越强；电平低于阈值时 `duck_scale` 恢复 1.0。
- 稳态参考：threshold=-20dB、ratio=4，侧链满幅(1.0) → `duck_scale≈0.1778`（-15dB）；电平 0.5 → ≈0.30。

## 总线回调

`AudioSource::OnBusGainChanged(effective_gain)` 在总线有效增益变化时被调用，
用于把 `总线增益 × 源增益` 写回 OpenAL 的 `AL_GAIN`。`OnBusDestroyed()` 在总线析构时解除关联。
这些回调由 `AudioBus` 内部维护，使用者一般无需直接处理。
