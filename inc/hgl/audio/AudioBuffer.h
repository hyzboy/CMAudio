#pragma once

#include<hgl/io/InputStream.h>
#include<hgl/audio/AudioFileType.h>
#include<hgl/log/Log.h>

namespace hgl
{
    using namespace io;

    /**
    * AudioBuffer是一个简单的音频数据管理类
    */
    class AudioBuffer                                                                               ///音频数据缓冲区类
    {
        OBJECT_LOGGER

        bool ok;

        void InitPrivate();

    private:

        uint      Index;
        double    Time;                                                                             ///<缓冲区中音频数据可以播放的时间(秒)
        uint      Size;                                                                             ///<缓冲区中音频数据的总字节数
        uint      Freq;                                                                             ///<音频数量采样率

    public:

        const uint      GetIndex()const{return Index;}
        const double    GetTime()const{return Time;}
        const uint      GetSize()const{return Size;}
        const uint      GetFreq()const{return Freq;}

    public:

        AudioBuffer(void *,int,AudioFileType);                                                      ///<本类构造函数
        AudioBuffer(InputStream *,int,AudioFileType);                                               ///<本类构造函数
        AudioBuffer(const os_char *filename=0,AudioFileType=AudioFileType::None);                   ///<本类构造函数
        virtual ~AudioBuffer();                                                                     ///<本类析构函数

        bool SetData(uint format,const void *data,uint size,uint freq);

        bool Load(void *,int,AudioFileType);                                                        ///<从内存中加载音频数据
        bool Load(InputStream *,int,AudioFileType);                                                 ///<从流中加载音频数据
        bool Load(const os_char *,AudioFileType=AudioFileType::None);                               ///<从文件中加载音频数据

        void Clear();                                                                               ///<清除数据
    };//class AudioBuffer

//  typedef ObjectBuffer<AudioBuffer>           AudioBufferBuffer;                                  ///<AudioBuffer缓冲管理器
}//namespace hgl
