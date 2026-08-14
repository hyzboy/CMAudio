#pragma once

#include<hgl/type/String.h>
#include<hgl/type/UnorderedSet.h>

namespace hgl::audio
{
    class AudioSource;

    /**
    * 音频总线节点，构成一棵树（Master → Music/SFX/Ambient/UI → ...）。
    * 有效增益 = 本节点增益 × 父链所有节点增益；静音沿子树向下传播。
    */
    class AudioBus
    {
        AudioBus   *parent;                        ///< 父总线（根为 nullptr）
        AnsiString  name;                          ///< 总线名称

        UnorderedSet<AudioBus    *> children;      ///< 子总线
        UnorderedSet<AudioSource *> sources;       ///< 直接挂载在此总线上的音源

        float gain;                                ///< 本节点增益（0.0=静音，1.0=满）
        bool  mute;                                ///< 静音标志
        float cached_effective_gain;               ///< 缓存的有效增益（已含父链与静音）

        void PropagateEffectiveGain(float parent_gain);    ///< 自顶向下重算有效增益并推送

    public:

        explicit AudioBus(AudioBus *parent=nullptr);
        ~AudioBus();

        AudioBus         *GetParent()const{return parent;}
        const AnsiString &GetName  ()const{return name;}

        void    SetGain(float);                     ///< 设置本节点增益
        float   GetGain()const{return gain;}
        void    SetMute(bool);                     ///< 设置静音
        bool    IsMute()const{return mute;}

        float   GetEffectiveGain()const{return cached_effective_gain;}    ///< 取得有效增益（含父链与静音）

        AudioBus *CreateChild(const char *name);    ///< 创建并挂接一个子总线

        void    AttachSource(AudioSource *);        ///< 挂接音源（由 AudioSource::SetBus 调用）
        void    DetachSource(AudioSource *);        ///< 解除音源挂接
    };//class AudioBus
}//namespace hgl::audio
