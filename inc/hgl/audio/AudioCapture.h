#pragma once

#include<hgl/al/al.h>
#include<hgl/al/alc.h>
#include<hgl/CoreType.h>

namespace hgl::audio
{
    /**
    * 音频录音（P2-3）：封装 OpenAL 的 alcCapture 系列函数指针
    * 用于语音聊天、卡拉OK、语音指令等录音场景。
    *
    * 用法：
    *   AudioCapture capture;
    *   if(capture.Open()) { capture.Start(); ... capture.ReadSamples(buf, n); capture.Stop(); }
    */
    class AudioCapture
    {
        openal::ALCdevice *device;          ///< 录音设备
        uint sample_rate;                   ///< 采样率
        uint sample_format;                 ///< 采样格式（AL_FORMAT_MONO16 等）
        uint buffer_size;                   ///< 内部缓冲样本数

    public:

        AudioCapture();
        ~AudioCapture();

        /**
        * 打开默认录音设备
        * @param sample_rate 采样率（默认 44100）
        * @param format 采样格式（默认 AL_FORMAT_MONO16）
        * @param buffer_seconds 内部缓冲时长（秒，默认 1.0）
        * @return 是否打开成功（无录音设备返回 false）
        */
        bool Open(uint sample_rate=44100,uint format=AL_FORMAT_MONO16,double buffer_seconds=1.0);

        void Close();                       ///< 关闭录音设备
        bool IsOpen()const{return device!=nullptr;}

        bool Start();                       ///< 开始录音
        bool Stop();                        ///< 停止录音

        int  GetAvailableSamples()const;    ///< 可取用的样本数（0=暂无）
        int  ReadSamples(void *buffer,int sample_count);    ///< 读样本，返回实际读取数

        uint GetSampleRate()const{return sample_rate;}
        uint GetSampleFormat()const{return sample_format;}
        uint GetBufferSize()const{return buffer_size;}
    };//class AudioCapture
}//namespace hgl::audio
