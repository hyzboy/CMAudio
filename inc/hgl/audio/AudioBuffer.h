#pragma once

#include<hgl/io/InputStream.h>
#include<hgl/audio/AudioFileType.h>
#include<hgl/audio/AudioMixerTypes.h>
#include<hgl/audio/ParametricEQ.h>
#include<hgl/thread/Atomic.h>
#include<hgl/log/Log.h>

namespace hgl::audio
{
    using io::InputStream;

    /**
    * AudioBuffer是一个简单的音频数据管理类
    */
    class AudioBuffer                                                                               ///音频数据缓冲区类
    {
        OBJECT_LOGGER

        bool loaded;

        void InitPrivate();

    private:

        uint      buffer_id;
        double    duration;                                                                             ///<缓冲区中音频数据可以播放的时间(秒)
        uint      data_size;                                                                             ///<缓冲区中音频数据的总字节数
        uint      sample_rate;                                                                             ///<音频数量采样率

        atom<uint> ref_count;                                                                           ///<引用计数（由 AudioAssetManager 管理）

        ParametricEQ eq;                                                                                ///<buffer 级参数化 EQ（P2：解码后/上传前应用，EFX 缺失兜底）

    public:

        uint            GetIndex()const{return buffer_id;}
        double          GetTime()const{return duration;}
        uint            GetSize()const{return data_size;}
        uint            GetFreq()const{return sample_rate;}
        bool            IsLoaded()const{return loaded;}                                                  ///<缓冲区是否已成功加载数据

        uint            GetRefCount()const{return ref_count.load();}                                      ///<获取当前引用计数
        uint            IncRef();                                                                         ///<引用计数+1，返回新值
        uint            DecRef();                                                                         ///<引用计数-1，返回新值

        ParametricEQ &  GetEQ(){return eq;}                                                              ///<取得 buffer 级参数化 EQ（Load/SetData 时应用）
        const ParametricEQ &GetEQ()const{return eq;}
        void            ClearEQ(){eq.ClearBands();}                                                      ///<清空 EQ 频段（等效直通）

    public:

        AudioBuffer(void *,int,AudioFileType);                                                      ///<本类构造函数
        AudioBuffer(InputStream *,int,AudioFileType);                                               ///<本类构造函数
        AudioBuffer(const os_char *filename=0,AudioFileType=AudioFileType::None);                   ///<本类构造函数
        virtual ~AudioBuffer();                                                                     ///<本类析构函数

        bool SetData(uint format,const void *data,uint size,uint freq);
        bool SetData(const AudioDataInfo &info,const void *data);                              ///<使用音频数据信息设置数据

        bool Load(void *,int,AudioFileType);                                                        ///<从内存中加载音频数据
        bool Load(InputStream *,int,AudioFileType);                                                 ///<从流中加载音频数据
        bool Load(const os_char *,AudioFileType=AudioFileType::None);                               ///<从文件中加载音频数据

        void Clear();                                                                               ///<清除数据
    };//class AudioBuffer

//  typedef ObjectBuffer<AudioBuffer>           AudioBufferBuffer;                                  ///<AudioBuffer缓冲管理器
}//namespace hgl::audio
