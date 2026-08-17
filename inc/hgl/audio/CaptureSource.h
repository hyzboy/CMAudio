#pragma once

#include<hgl/CoreType.h>
#include<hgl/al/al.h>
#include<hgl/al/alc.h>

namespace hgl::audio
{
    /**
    * 实时捕获源（P0：录音伪装解码器）
    *
    * 封装 OpenAL 录音设备（alcCapture 系列），提供"帧式"读取，
    * 与 AudioPlayer 的流式播放管线对接，实现 Live 录音/语音输入。
    *
    * - 帧式读取：ReadFrame 按固定帧长（如 20ms）轮询取数，保证每帧完整
    * - mock 模式：无录音设备时内部合成测试信号（开发/CI 用）
    *
    * 用法：
    *   CaptureSource src;
    *   if(src.Open(16000, 20)) { src.Start(); src.ReadFrame(buf, n); src.Stop(); }
    */
    class CaptureSource
    {
        openal::ALCdevice *device;          ///< 录音设备
        uint sample_rate;                   ///< 采样率
        uint sample_format;                 ///< 采样格式（AL_FORMAT_MONO16 等）
        uint frame_samples;                 ///< 每帧样本数（帧长 = frame_ms）
        uint buffer_size;                   ///< 设备环形缓冲样本数
        bool mock;                          ///< 模拟模式（内部合成信号）
        uint mock_phase;                    ///< 模拟正弦相位（样本计数）

    public:

        CaptureSource();
        ~CaptureSource();

        /**
        * 打开录音源
        * @param sample_rate 采样率（通话 16000 / 变声 48000）
        * @param frame_ms 帧长（毫秒，通话 20 / 变声 10）
        * @param format 采样格式（默认 AL_FORMAT_MONO16）
        * @param use_mock 无录音设备时用内部合成源（默认 false；true 则无设备也成功）
        * @return 是否打开成功
        */
        bool Open(uint sample_rate=16000,uint frame_ms=20,uint format=AL_FORMAT_MONO16,bool use_mock=false);
        void Close();                                                           ///< 关闭录音源
        bool IsOpen()const{return device!=nullptr||mock;}                       ///< 是否已打开

        bool Start();                                                           ///< 开始采集
        bool Stop();                                                            ///< 停止采集

        int  GetAvailableSamples()const;                                        ///< 当前可取样本数（mock=帧样本数）
        int  ReadFrame(void *buffer,int max_samples);                           ///< 读一帧，返回字节数（0=暂无数据）

        uint GetSampleRate()const{return sample_rate;}
        uint GetFormat()const{return sample_format;}
        uint GetFrameSamples()const{return frame_samples;}
        uint GetFrameBytes()const{return frame_samples*((sample_format==AL_FORMAT_MONO8)?1:2);}
    };//class CaptureSource
}//namespace hgl::audio
