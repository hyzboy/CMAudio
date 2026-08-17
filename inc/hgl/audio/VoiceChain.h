#pragma once

#include<hgl/CoreType.h>
#include<hgl/audio/PitchShifter.h>
#include<hgl/audio/ParametricEQ.h>
#include<hgl/audio/Compressor.h>
#include<hgl/audio/TimeEffects.h>
#include<vector>

namespace hgl::audio
{
    /**
    * 变声器预设
    */
    enum class VoicePreset
    {
        None=0,         ///< 直通（仅基础处理，无效果）
        Robot,          ///< 机器人：半波门控调制（80Hz 方波）
        Helium,         ///< 氦气：+12 半音升调 + 高频 shelf
        MegaPhone,      ///< 扩音器：带通 + 高压缩 + 补偿增益
        Chorus          ///< 合唱：轻微升调 + Chorus 湿信号
    };//enum class VoicePreset

    /**
    * 实时变声效果链（P1）
    *
    * 编排器：PitchShifter（变调）→ [环形调制 Robot] → ParametricEQ → Chorus → Compressor。
    * 流式接口（输入一帧 → 输出一帧），挂载在实时录音播放管线（CaptureSource → AudioPlayer）之后。
    *
    * 用法：
    *   VoiceChain vc;
    *   vc.Init(48000);
    *   vc.SetPreset(VoicePreset::Helium);
    *   vc.Process(cap_frame, n);            // 输入捕获帧
    *   while(vc.GetOutputCount()>0) vc.ReadOutput(out, n);
    */
    class VoiceChain
    {
        PitchShifter  shifter;              ///< 变调（WSOLA + 重采样）
        ParametricEQ  eq;                   ///< 滤波
        Chorus        chorus;               ///< 合唱（TimeEffects）
        Compressor    comp;                 ///< 压缩

        bool  mod_enabled;                  ///< 环形调制开关（Robot）
        float mod_freq;                     ///< 调制频率（Hz）
        float mod_phase;                    ///< 调制相位（0..1）

        bool chorus_enabled;                ///< Chorus 开关

        float sample_rate;

        std::vector<float> tmp_buf;         ///< 中间缓冲
        std::vector<float> out_buf;         ///< 输出 FIFO

    public:

        VoiceChain();

        void Init(float sample_rate);

        void SetPreset(VoicePreset preset); ///< 切换预设（自动 Reset 处理链）
        VoicePreset GetPreset()const{return preset;}

        void SetPitch(float pitch);         ///< 手动变调（0.5-2.0，预设之上微调）

        void Reset();

        void Process(const float *in,int count);
        int  GetOutputCount()const{return (int)out_buf.size();}
        int  ReadOutput(float *out,int max_count);

    private:

        VoicePreset preset;
    };//class VoiceChain
}//namespace hgl::audio
