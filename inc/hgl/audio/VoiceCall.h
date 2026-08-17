#pragma once

#include<hgl/CoreType.h>
#include<hgl/audio/VoicePreprocess.h>
#include<hgl/audio/AudioCodec.h>
#include<hgl/audio/JitterBuffer.h>
#include<vector>

namespace hgl::audio
{
    /**
    * 语音通话链（P3）：CaptureSource → 预处理(NS/AGC/VAD) → Opus 编码 → 抖动缓冲 → Opus 解码 → 播放
    *
    * 本机环回编排器：Send() 走发送链（预处理→编码→入抖动缓冲），
    * Receive() 走接收链（抖动缓冲→解码→输出），可模拟网络丢包（PLC 隐藏）。
    *
    * 用法：
    *   VoiceCall call;
    *   call.Start(16000,1,32000);
    *   bool speech=call.Send(pcm,320);          // 返回 VAD
    *   call.SetLossRate(0.1f);                   // 模拟 10% 丢包
    *   bool got=call.Receive(out,320);           // 输出解码 PCM
    */
    class VoiceCall
    {
        VoicePreprocess  preprocess;    ///< 输入端预处理（NS→AGC→VAD）
        AudioCodec       codec;         ///< 编解码器（Opus 插件）
        JitterBuffer     jitter;        ///< 接收端抖动缓冲

        uint32 seq;                     ///< 发送端序列号
        uint sample_rate;
        uint frame_samples;

        float loss_rate;                ///< 模拟丢包率（0..1）
        std::vector<char> packet_buf;   ///< 编码包缓冲

    public:

        VoiceCall();

        /**
        * 启动通话链
        * @param plugin_name 编码插件名（OS_TEXT("Opus")）
        * @param sample_rate 采样率（通话 16000）
        * @param channels 声道（1）
        * @param bitrate 码率 bps
        * @param frame_ms 帧长毫秒（默认 20）
        * @return 是否成功
        */
        bool Start(const OSString &plugin_name,uint sample_rate,uint channels,uint bitrate,uint frame_ms=20);

        void Stop();

        bool IsRunning()const{return codec.IsOpen();}

        void SetLossRate(float rate){loss_rate=(rate<0)?0.0f:((rate>1)?1.0f:rate);}   ///< 模拟网络丢包率
        float GetLossRate()const{return loss_rate;}

        /**
        * 发送一帧（发送链：预处理→编码→入抖动缓冲）
        * @param pcm 输入 PCM（frame_samples 样本）
        * @param frame_samples 帧样本数
        * @return VAD：本帧是否语音
        */
        bool Send(const float *pcm,uint frame_samples);

        /**
        * 接收一帧（接收链：抖动缓冲→解码；丢包走 PLC）
        * @param out 输出 PCM（frame_samples 样本）
        * @param frame_samples 帧样本数
        * @return 是否有有效输出（丢包 PLC 也算 true；未启动 false）
        */
        bool Receive(float *out,uint frame_samples);

        VoicePreprocess &GetPreprocess(){return preprocess;}
        JitterBuffer &GetJitter(){return jitter;}
    };//class VoiceCall
}//namespace hgl::audio
