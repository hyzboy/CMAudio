#include<hgl/audio/AudioBus.h>
#include<hgl/audio/AudioSource.h>

namespace hgl::audio
{
    AudioBus::AudioBus(AudioBus *p)
    {
        parent=p;
        gain=1.0f;
        mute=false;
        duck_scale=1.0f;
        cached_effective_gain=1.0f;
        sidechain_enabled=false;
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

    void AudioBus::RecalculateSubtree()
    {
        cached_effective_gain=ParentGain()*(mute?0.0f:gain)*duck_scale;

        for(AudioSource *s : sources)
            s->OnBusGainChanged(cached_effective_gain);

        for(AudioBus *child : children)
            child->RecalculateSubtree();
    }

    void AudioBus::SetGain(float g)
    {
        if(g<0.0f)g=0.0f;

        if(gain==g)return;

        gain=g;
        RecalculateSubtree();       // 本节点与子树受影响，父链不变
    }

    void AudioBus::SetMute(bool m)
    {
        if(mute==m)return;

        mute=m;
        RecalculateSubtree();
    }

    void AudioBus::Duck(float target_scale,double duration,double now)
    {
        if(target_scale<0.0f)target_scale=0.0f;
        if(target_scale>1.0f)target_scale=1.0f;

        if(duration<=0.0)           // 无过渡：立即生效
        {
            if(duck_scale==target_scale)return;

            duck_scale=target_scale;
            RecalculateSubtree();
            return;
        }

        duck_ramp.Start(now,duck_scale,target_scale,duration);
        Update(now);                // 立即推进一次，让 duck_scale 开始变化
    }

    void AudioBus::Unduck(double duration,double now)
    {
        Duck(1.0f,duration,now);
    }

    void AudioBus::SetSidechainDuck(float sr,float threshold_db,float ratio,float attack_sec,float release_sec)
    {
        sidechain_comp.SetSampleRate(sr);
        sidechain_comp.Configure({.threshold_db=threshold_db, .ratio=ratio,
                                  .attack_sec=attack_sec, .release_sec=release_sec});
        sidechain_comp.Reset();
        sidechain_enabled=true;
    }

    void AudioBus::UpdateSidechainDuck(float sidechain_level)
    {
        if(!sidechain_enabled)return;

        const float gain=sidechain_comp.UpdateFromLevel(sidechain_level);

        if(gain!=duck_scale)
        {
            duck_scale=gain;
            RecalculateSubtree();
        }
    }

    void AudioBus::DisableSidechainDuck()
    {
        sidechain_enabled=false;
        Duck(1.0f,0.0,0.0);         // 立即恢复
    }

    void AudioBus::Update(const double now)
    {
        if(duck_ramp.active)
        {
            float new_scale;

            duck_ramp.Evaluate(now,new_scale);

            if(new_scale!=duck_scale)
            {
                duck_scale=new_scale;
                RecalculateSubtree();
            }
        }

        for(AudioBus *child : children)
            child->Update(now);
    }

    AudioBus *AudioBus::CreateChild(const char *n)
    {
        AudioBus *child=new AudioBus(this);

        if(n)child->name=n;

        children.Add(child);

        child->RecalculateSubtree();    // 继承父链有效增益

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
}//namespace hgl::audio
