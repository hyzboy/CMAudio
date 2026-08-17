#include<hgl/audio/JitterBuffer.h>
#include<cstring>

namespace hgl::audio
{
    JitterBuffer::JitterBuffer()
    {
        next_seq=0;
        max_buffered=5;
        started=false;
    }

    void JitterBuffer::Reset(uint32 start_seq)
    {
        buf.clear();
        next_seq=start_seq;
        started=false;
    }

    bool JitterBuffer::Push(uint32 seq,const char *data,int size)
    {
        if(!data||size<=0)
            return(false);

        if(started)
        {
            if(seq<next_seq)            // 迟到包（已过播放时钟）
                return(false);
        }
        else
        {
            next_seq=seq;               // 首包：播放时钟从此开始
            started=true;
        }

        // 缓冲上限：丢弃最旧（防极端延迟下缓冲无限增长）
        while((uint)buf.size()>=max_buffered)
            buf.erase(buf.begin());

        Entry e;
        e.data.assign(data,data+size);
        buf[seq]=std::move(e);

        return(true);
    }

    int JitterBuffer::Poll(char *out,int out_cap)
    {
        if(!out||out_cap<=0)
            return(0);

        auto it=buf.find(next_seq);

        if(it!=buf.end())
        {
            const int n=(int)it->second.data.size();

            if(n<=out_cap)
                memcpy(out,it->second.data.data(),n);

            buf.erase(it);
            next_seq++;
            return(n);
        }

        // 无帧：丢包或尚未到达。播放时钟照常推进（调用方走 PLC 隐藏）
        next_seq++;
        return(0);
    }
}//namespace hgl::audio
