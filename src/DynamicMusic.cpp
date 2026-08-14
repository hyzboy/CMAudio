#include<hgl/audio/DynamicMusic.h>
#include<hgl/audio/AudioPlayer.h>

namespace hgl::audio
{
    DynamicMusic::DynamicMusic()
    {
        current_state=-1;
    }

    DynamicMusic::~DynamicMusic()
    {
    }

    int DynamicMusic::AddLayer(AudioPlayer *player,float base_gain)
    {
        Layer layer;

        layer.player=player;
        layer.base_gain=base_gain;
        layer.current_gain=base_gain;
        layer.target_gain=base_gain;

        layers.push_back(layer);

        return (int)layers.size()-1;
    }

    int DynamicMusic::AddState(const os_char *name,const float *gains,int count)
    {
        State state;

        if(name)state.name=name;

        for(int i=0;i<count;i++)
            state.gains.Add(gains?gains[i]:1.0f);

        states.push_back(state);

        return (int)states.size()-1;
    }

    bool DynamicMusic::SetState(int index,double now,double crossfade)
    {
        if(index<0||index>=(int)states.size())return false;

        State &state=states[index];

        current_state=index;

        const int layer_count=(int)layers.size();
        const int gain_count =state.gains.GetCount();

        for(int i=0;i<layer_count;i++)
        {
            Layer &layer=layers[i];

            const float state_gain=(i<gain_count)?(state.gains[i]):1.0f;
            const float target=layer.base_gain*state_gain;

            layer.target_gain=target;

            if(crossfade<=0.0)          // 无过渡：立即生效
            {
                layer.current_gain=target;
                layer.ramp.active=false;

                if(layer.player)
                    layer.player->SetGain(target);
            }
            else
            {
                layer.ramp.Start(now,layer.current_gain,target,crossfade);
            }
        }

        return true;
    }

    bool DynamicMusic::SetState(const os_char *name,double now,double crossfade)
    {
        if(!name)return false;

        for(int i=0;i<(int)states.size();i++)
        {
            if(states[i].name==OSString(name))
                return SetState(i,now,crossfade);
        }

        return false;
    }

    void DynamicMusic::Update(double now)
    {
        for(Layer &layer : layers)
        {
            float g;

            layer.ramp.Evaluate(now,g);

            if(g!=layer.current_gain)
            {
                layer.current_gain=g;

                if(layer.player)
                    layer.player->SetGain(g);
            }
        }
    }

    float DynamicMusic::GetLayerGain(int layer)const
    {
        if(layer<0||layer>=(int)layers.size())return 0.0f;

        return layers[layer].current_gain;
    }

    AudioPlayer *DynamicMusic::GetLayerPlayer(int layer)const
    {
        if(layer<0||layer>=(int)layers.size())return nullptr;

        return layers[layer].player;
    }
}//namespace hgl::audio
