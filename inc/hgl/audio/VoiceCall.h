#pragma once

#include<hgl/CoreType.h>
#include<hgl/audio/VoicePreprocess.h>
#include<hgl/audio/AudioCodec.h>
#include<hgl/audio/JitterBuffer.h>
#include<hgl/audio/CaptureSource.h>
#include<hgl/audio/AudioSessionPolicy.h>
#include<vector>

namespace hgl::audio
{
    /**
    * 语音通话链（P3）：CaptureSource → 预处理(NS/AGC/VAD) → Opus 编码 → 抖动缓冲 → Opus 解码 → 播放
    *
    * 两种驱动方式：
    * - 内存环回：Send()/Receive() 手动喂帧（测试/模拟网络）
    * - 设备模式（P4）：AttachCapture 挂录音源 + SetOutputCallback 挂输出，
    *   每帧调 UpdateDevice() 驱动一帧真实设备通话（采集→编码→解码→回调）。
    *
    * 会话策略联动（P4）：SetSessionPolicy 后 Start 自动 RequestFocus；
    * UpdateDevice 时失焦/被静音则输出静音帧（通话挂起）。
    */
    class VoiceCall
    {
    public:
        /**
        * 解码输出回调（设备模式）
        * @param pcm 解码后 float PCM（-1..1）
        * @param frame_samples 帧样本数
        * @param user_data 用户数据（SetOutputCallback 传入）
        */
        typedef void (*OutputCallback)(const float *pcm,uint frame_samples,void *user_data);

    private:
        VoicePreprocess  preprocess;    ///< 输入端预处理（NS→AGC→VAD）
        AudioCodec       codec;         ///< 编解码器（Opus 插件）
        JitterBuffer     jitter;        ///< 接收端抖动缓冲

        uint32 seq;                     ///< 发送端序列号
        uint sample_rate;
        uint frame_samples;

        float loss_rate;                ///< 模拟丢包率（0..1）
        std::vector<char> packet_buf;   ///< 编码包缓冲

        // P4 设备集成
        CaptureSource    *capture;      ///< 录音源（设备模式发送端，外部持有）
        AudioSessionPolicy *policy;     ///< 会话策略（外部持有，可空）
        OutputCallback   out_cb;        ///< 解码输出回调（设备模式接收端）
        void             *out_user;     ///< 回调用户数据
        bool             device_active; ///< 设备模式是否在通话中（焦点挂起标志）

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

        // ---- P4 设备集成 ----

        void AttachCapture(CaptureSource *src){capture=src;}    ///< 挂录音源（设备模式发送端，外部持有）
        void SetOutputCallback(OutputCallback cb,void *user=nullptr){out_cb=cb;out_user=user;}  ///< 挂解码输出回调
        void SetSessionPolicy(AudioSessionPolicy *p){policy=p;} ///< 挂会话策略（焦点/静音联动，可空）

        /**
        * 驱动一帧设备通话（须已 AttachCapture + Start）
        * 采集 int16 → float → Send（预处理+编码）→ Receive（抖动+解码）→ 输出回调
        * 会话策略：失焦/被静音时输出静音帧并跳过采集（通话挂起）
        * @return 是否完成一帧（false=未启动/无录音源/设备无数据）
        */
        bool UpdateDevice();

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
