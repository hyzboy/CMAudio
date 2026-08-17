#include<hgl/audio/WSOLAShifter.h>
#include<cmath>

namespace hgl::audio
{
    namespace
    {
        constexpr float WS_PI = 3.14159265358979323846f;
    }//namespace

    WSOLAShifter::WSOLAShifter()
    {
        sample_rate=48000.0f;
        stretch=1.0f;
        W=1920;
        overlap=960;
        delta_in=1920;
        search=480;
        in_anchor=0;
        first_frame=true;
    }

    void WSOLAShifter::Init(float sr,float stretch_ratio,float win_ms,float search_ms)
    {
        sample_rate=sr;

        W=(int)(sr*win_ms/1000.0f);

        if(W<8)W=8;

        overlap=W/2;

        if(overlap<4)overlap=4;

        search=(int)(sr*search_ms/1000.0f);

        if(search<1)search=1;

        stretch=stretch_ratio;

        if(stretch<0.5f)stretch=0.5f;
        if(stretch>2.0f)stretch=2.0f;

        delta_in=(int)((float)W/stretch+0.5f);

        Reset();
    }

    void WSOLAShifter::SetStretch(float ratio)
    {
        stretch=ratio;

        if(stretch<0.5f)stretch=0.5f;
        if(stretch>2.0f)stretch=2.0f;

        delta_in=(int)((float)W/stretch+0.5f);
    }

    void WSOLAShifter::Reset()
    {
        in_buf.clear();
        out_buf.clear();
        mid_buf.assign(W,0.0f);
        in_anchor=0;
        first_frame=true;
    }

    int WSOLAShifter::FindBest(int lo,int hi)
    {
        int best=lo;
        float best_ncc=-1.0e9f;

        for(int t=lo;t<=hi;t++)
        {
            float dot=0,ea=0,eb=0;

            for(int i=0;i<overlap;i++)
            {
                const float a=mid_buf[i];
                const float b=in_buf[t+i];

                dot+=a*b;
                ea+=a*a;
                eb+=b*b;
            }

            if(ea<1.0e-12f||eb<1.0e-12f)
                continue;

            const float ncc=dot/std::sqrt(ea*eb);

            if(ncc>best_ncc)
            {
                best_ncc=ncc;
                best=t;
            }
        }

        return best;
    }

    void WSOLAShifter::ProduceFrame()
    {
        const int ideal=in_anchor;              // 本帧搜索中心

        int best;

        if(first_frame)
        {
            // 首帧：无衔接参考，直接取窗口起点
            best=0;
            first_frame=false;
        }
        else
        {
            int lo=ideal-search;
            int hi=ideal+search;

            if(lo<0)lo=0;

            best=FindBest(lo,hi);
        }

        // 1. 输出窗口前 overlap：pMidBuffer 与 输入片段 等功率 crossfade
        for(int i=0;i<overlap;i++)
        {
            const float w=0.5f-0.5f*std::cos(WS_PI*(float)i/(float)(overlap-1));
            out_buf.push_back(mid_buf[i]*(1.0f-w)+in_buf[best+i]*w);
        }

        // 2. 输出窗口剩余 W-overlap：直接拷贝
        for(int i=overlap;i<W;i++)
            out_buf.push_back(in_buf[best+i]);

        // 3. 新 pMidBuffer = 窗口之后的 W 样本（下一帧衔接参考）
        mid_buf.assign(in_buf.begin()+best+W,in_buf.begin()+best+2*W);

        // 4. 下一帧搜索中心 = 本帧理想位置 + delta_in（每帧净消费 delta_in）
        in_anchor=ideal+delta_in;

        // 5. 裁剪输入缓冲：best 之前不再需要（下一帧窗口起点 >= ideal+delta_in-search > best）
        if(best>0)
        {
            in_buf.erase(in_buf.begin(),in_buf.begin()+best);
            in_anchor-=best;
        }
    }

    void WSOLAShifter::Process(const float *in,int count)
    {
        if(!in||count<=0)
            return;

        in_buf.insert(in_buf.end(),in,in+count);

        while((int)in_buf.size()>=NeededInput())
            ProduceFrame();
    }

    int WSOLAShifter::ReadOutput(float *out,int max_count)
    {
        if(!out||max_count<=0)
            return 0;

        const int n=(max_count<(int)out_buf.size())?max_count:(int)out_buf.size();

        for(int i=0;i<n;i++)
            out[i]=out_buf[i];

        out_buf.erase(out_buf.begin(),out_buf.begin()+n);

        return n;
    }
}//namespace hgl::audio
