#pragma once

#include<hgl/CoreType.h>
#include<hgl/type/String.h>

namespace hgl::audio
{
    struct AudioCodecPlugInInterface;

    /**
    * 音频编解码器（P3：语音通话链编码端）
    *
    * 通过编码插件接口（ver=5）加载具体编码器插件（如 Opus），
    * 一个对象同时持有编码器+解码器句柄（双向通道），贴合通话链
    * CaptureSource→预处理→Encode→网络→Decode→播放 的使用形态。
    *
    * 编码能力是插件可选能力（同浮点解码"不是谁都有"）：
    * 插件未实现 ver=5 接口时 Open 返回 false。
    *
    * 用法：
    *   AudioCodec codec;
    *   if(codec.Open(OS_TEXT("Opus"),16000,1,32000))
    *   {
    *       int n=codec.Encode(pcm,320,packet,sizeof(packet));   // 编码一帧
    *       int m=codec.Decode(packet,n,out,320);                // 解码（packet=nullptr→PLC）
    *   }
    */
    class AudioCodec
    {
        AudioCodecPlugInInterface *iface;   ///< 编码插件接口（new 拷贝，生命周期自持）
        void *enc;                          ///< 编码器句柄
        void *dec;                          ///< 解码器句柄

    public:

        AudioCodec();
        ~AudioCodec();

        /**
        * 打开编解码器（自动创建编码器+解码器）
        * @param plugin_name 编码插件名（如 OS_TEXT("Opus")）
        * @param sample_rate 采样率（通话 16000）
        * @param channels 声道数（通话 1）
        * @param bitrate 码率 bps（0=编码器默认；Opus 语音建议 24000-32000）
        * @return 是否成功（插件无编码能力/参数非法返回 false）
        */
        bool Open(const OSString &plugin_name,uint sample_rate,uint channels,uint bitrate=0);

        void Close();                       ///< 关闭（自动释放编码器+解码器+接口）

        bool IsOpen()const{return iface&&enc&&dec;}     ///< 是否已打开

        /**
        * 编码一帧 PCM→压缩包
        * @param pcm 输入 float PCM（-1..1）
        * @param frame_samples 帧样本数（Opus 需为 2.5/5/10/20/40/60ms 对应样本数）
        * @param packet 输出压缩包缓冲
        * @param packet_cap 压缩包缓冲容量（建议 ≥4000）
        * @return 压缩包字节数；负数=编码错误码
        */
        int Encode(const float *pcm,uint frame_samples,char *packet,uint packet_cap);

        /**
        * 解码一包压缩数据→PCM
        * @param packet 压缩包（nullptr=丢包，走 PLC 隐藏）
        * @param packet_size 压缩包字节数（packet=nullptr 时忽略）
        * @param pcm 输出 float PCM 缓冲
        * @param pcm_cap 输出缓冲样本数容量
        * @return 解码样本数；负数=解码错误码
        */
        int Decode(const char *packet,int packet_size,float *pcm,uint pcm_cap);
    };//class AudioCodec
}//namespace hgl::audio
