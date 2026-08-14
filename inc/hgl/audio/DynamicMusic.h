#pragma once

#include<hgl/type/String.h>
#include<hgl/type/ValueArray.h>
#include<hgl/audio/GainEnvelope.h>
#include<vector>

namespace hgl::audio
{
    class AudioPlayer;

    /**
    * 动态音乐分层（P2-2）：多个音乐层（stem）同时播放，
    * 游戏状态（战斗/探索/...）决定各层的目标音量，状态切换时平滑 crossfade。
    *
    * 用法：
    *   DynamicMusic music;
    *   music.AddLayer(bgm_drums, 1.0f);
    *   music.AddLayer(bgm_melody, 1.0f);
    *   float explore[]={1.0f,0.2f};  music.AddState("explore", explore, 2);
    *   float combat []={1.0f,1.0f};  music.AddState("combat",  combat,  2);
    *   music.SetState("explore", now, 2.0);
    *   ...每帧 music.Update(now);
    */
    class DynamicMusic
    {
    public:

        struct Layer
        {
            AudioPlayer *player;        ///< 播放器（可为 nullptr，用于纯逻辑测试）
            float base_gain;            ///< 层基础增益
            float current_gain;         ///< 当前实际增益
            float target_gain;          ///< 目标增益（由状态决定）
            GainRamp ramp;              ///< crossfade 过渡斜坡
        };

        struct State
        {
            OSString name;              ///< 状态名
            ValueArray<float> gains;    ///< 各层的状态增益系数（0=静音，1=满）
        };

    private:

        std::vector<Layer> layers;      ///< 音乐层
        std::vector<State> states;      ///< 状态
        int current_state;              ///< 当前状态（-1=无）

    public:

        DynamicMusic();
        ~DynamicMusic();

        int AddLayer(AudioPlayer *player,float base_gain=1.0f);            ///< 添加音乐层，返回层索引
        int AddState(const os_char *name,const float *gains,int count);    ///< 添加状态，返回状态索引

        bool SetState(const os_char *name,double now,double crossfade=1.0);///< 按名切换状态
        bool SetState(int index,double now,double crossfade=1.0);          ///< 按索引切换状态

        void Update(double now);                                            ///< 每帧驱动 crossfade

        int   GetLayerCount()const{return (int)layers.size();}
        int   GetStateCount()const{return (int)states.size();}
        int   GetCurrentState()const{return current_state;}
        float GetLayerGain(int layer)const;                                 ///< 某层当前实际增益
        AudioPlayer *GetLayerPlayer(int layer)const;                        ///< 某层播放器
    };//class DynamicMusic
}//namespace hgl::audio
