#pragma once

#include<hgl/CoreType.h>
#include<map>
#include<vector>

namespace hgl::audio
{
    /**
    * 抖动缓冲（JitterBuffer，P3）
    *
    * 语音通话接收端：网络包到达时间不均（抖动）/乱序/迟到，本缓冲按序列号重排，
    * 提供稳定的"每帧一个"播放节奏；丢包时返回空（由解码端走 PLC 隐藏）。
    *
    * 语义：
    * - Push(seq,data)：入缓冲；序列号早于播放时钟的迟到包直接丢弃
    * - Poll()：取下一帧；播放时钟恒推进（无帧=丢包，调用方走 PLC）
    * - 超前包缓冲最多 max_buffered 帧，超出丢最旧（防延迟无限增长）
    */
    class JitterBuffer
    {
        struct Entry
        {
            std::vector<char> data;
        };

        std::map<uint32,Entry> buf;     ///< 序列号→包
        uint32 next_seq;                ///< 播放时钟：期望的下一帧序列号
        uint max_buffered;              ///< 最大缓冲帧数（超前包上限）
        bool started;                   ///< 是否已启动（首包决定播放时钟起点）

    public:

        JitterBuffer();

        void Reset(uint32 start_seq=0);         ///< 重置（可指定首帧序列号）

        void SetMaxBuffered(uint n){max_buffered=n;}    ///< 最大缓冲帧数（默认 5=100ms @20ms/帧）

        /**
        * 入缓冲
        * @param seq 序列号（发送端每帧 +1）
        * @param data 压缩包数据
        * @param size 压缩包字节数
        * @return 是否入缓冲成功（false=迟到包被丢弃）
        */
        bool Push(uint32 seq,const char *data,int size);

        /**
        * 取下一帧（播放时钟推进）
        * @return 包字节数；0=无帧（丢包，调用方应走 PLC）
        */
        int Poll(char *out,int out_cap);

        uint32 GetNextSeq()const{return next_seq;}      ///< 播放时钟
        int GetBufferedCount()const{return (int)buf.size();}    ///< 当前缓冲帧数
        bool IsStarted()const{return started;}
    };//class JitterBuffer
}//namespace hgl::audio
