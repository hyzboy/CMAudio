#include<hgl/audio/AudioBus.h>
#include<hgl/audio/AudioSource.h>

namespace hgl::audio
{
    AudioBus::AudioBus(AudioBus *p)
    {
        parent=p;
        gain=1.0f;
        mute=false;
        cached_effective_gain=1.0f;
    }

    AudioBus::~AudioBus()
    {
        // 解除所有挂载音源的关联（音源本身不归总线所有，仅重置其总线引用）
        for(AudioSource *s : sources)
            s->OnBusDestroyed();

        // 递归释放子总线
        for(AudioBus *child : children)
            delete child;
    }

    void AudioBus::SetGain(float g)
    {
        if(g<0.0f)g=0.0f;

        if(gain==g)return;

        gain=g;

        // 自根节点重算（本节点变更影响整棵子树）
        AudioBus *root=this;
        while(root->parent)root=root->parent;

        root->PropagateEffectiveGain(1.0f);
    }

    void AudioBus::SetMute(bool m)
    {
        if(mute==m)return;

        mute=m;

        AudioBus *root=this;
        while(root->parent)root=root->parent;

        root->PropagateEffectiveGain(1.0f);
    }

    AudioBus *AudioBus::CreateChild(const char *n)
    {
        AudioBus *child=new AudioBus(this);

        if(n)child->name=n;

        children.Add(child);

        child->cached_effective_gain=cached_effective_gain;    // 子节点默认增益 1.0，继承父链有效增益

        return child;
    }

    void AudioBus::AttachSource(AudioSource *s)
    {
        if(!s)return;

        sources.Add(s);
    }

    void AudioBus::DetachSource(AudioSource *s)
    {
        if(!s)return;

        sources.Delete(s);
    }

    void AudioBus::PropagateEffectiveGain(float parent_gain)
    {
        cached_effective_gain=parent_gain*(mute?0.0f:gain);

        for(AudioSource *s : sources)
            s->OnBusGainChanged(cached_effective_gain);

        for(AudioBus *child : children)
            child->PropagateEffectiveGain(cached_effective_gain);
    }
}//namespace hgl::audio
