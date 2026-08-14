#pragma once

#include<hgl/type/String.h>
#include<hgl/type/UnorderedSet.h>
#include<hgl/audio/GainEnvelope.h>
#include<hgl/audio/Compressor.h>

namespace hgl::audio
{
    class AudioSource;

    /**
    * 音频总线节点，构成一棵树（Master → Music/SFX/Ambient/UI → ...）。
    * 有效增益 = 父链有效增益 × 本节点增益 × 静音系数 × Duck 缩放。
    */
    class AudioBus
    {
        AudioBus   *parent;                        ///< 父总线（根为 nullptr）
        AnsiString  name;                          ///< 总线名称

        UnorderedSet<AudioBus    *> children;      ///< 子总线
        UnorderedSet<AudioSource *> sources;       ///< 直接挂载在此总线上的音源

        float gain;                                ///< 本节点增益（0.0=静音，1.0=满）
        bool  mute;                                ///< 静音标志
        float duck_scale;                          ///< Duck 缩放（1.0=无压低）
        GainRamp duck_ramp;                        ///< Duck 平滑过渡斜坡
        float cached_effective_gain;               ///< 缓存的有效增益（已含父链/静音/Duck）

        Compressor sidechain_comp;                 ///< 侧链压缩器（P3 延伸）
        bool sidechain_enabled;                    ///< 是否启用侧链压缩 Duck

        float ParentGain()const{return parent?parent->cached_effective_gain:1.0f;}
        void  RecalculateSubtree();                ///< 自本节点向下重算有效增益并推送

    public:

        explicit AudioBus(AudioBus *parent=nullptr);
        ~AudioBus();

        AudioBus         *GetParent()const{return parent;}
        const AnsiString &GetName  ()const{return name;}

        void    SetGain(float);                     ///< 设置本节点增益
        float   GetGain()const{return gain;}
        void    SetMute(bool);                     ///< 设置静音
        bool    IsMute()const{return mute;}

        float   GetEffectiveGain()const{return cached_effective_gain;}    ///< 取得有效增益（含父链/静音/Duck）

        // Ducking（侧链）：临时压低本总线（及子树）音量，用于"语音/重要音效压低音乐"
        void    Duck(float target_scale,double duration=0.2,double now=0);   ///< 平滑压低到 target_scale（0=完全压低，1=无）
        void    Unduck(double duration=0.2,double now=0);                     ///< 恢复
        float   GetDuckScale()const{return duck_scale;}                      ///< 取得当前 Duck 缩放
        bool    IsDucked()const{return duck_scale<1.0f-0.0001f;}             ///< 是否处于被压低状态
        void    Update(const double now);                                    ///< 驱动 Duck 平滑过渡（每帧调用，递归子树）

        // 侧链压缩 Duck（P3 延伸）：侧链信号电平动态驱动压缩，替代静态 Duck
        void    SetSidechainDuck(float sample_rate,float threshold_db=-20.0f,float ratio=4.0f,
                                 float attack_sec=0.01f,float release_sec=0.1f);
        void    UpdateSidechainDuck(float sidechain_level);                  ///< 每帧用侧链电平驱动（0=无信号，1=满幅）
        void    DisableSidechainDuck();                                      ///< 关闭侧链压缩，恢复 duck_scale=1.0
        bool    IsSidechainDuckEnabled()const{return sidechain_enabled;}

        AudioBus *CreateChild(const char *name);    ///< 创建并挂接一个子总线

        void    AttachSource(AudioSource *);        ///< 挂接音源（由 AudioSource::SetBus 调用）
        void    DetachSource(AudioSource *);        ///< 解除音源挂接
    };//class AudioBus
}//namespace hgl::audio
