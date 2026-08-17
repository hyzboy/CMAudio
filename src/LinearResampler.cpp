#include<hgl/audio/LinearResampler.h>

namespace hgl::audio
{
    LinearResampler::LinearResampler()
    {
        sample_rate=48000.0f;
        ratio=1.0f;
        pos=0.0;
        last_in=0.0f;
        has_last=false;
    }

    void LinearResampler::Init(float sr,float r)
    {
        sample_rate=sr;
        ratio=r;

        if(ratio<0.5f)ratio=0.5f;
        if(ratio>2.0f)ratio=2.0f;

        Reset();
    }

    void LinearResampler::SetRatio(float r)
    {
        ratio=r;

        if(ratio<0.5f)ratio=0.5f;
        if(ratio>2.0f)ratio=2.0f;
    }

    void LinearResampler::Reset()
    {
        pos=0.0;
        last_in=0.0f;
        has_last=false;
        out_buf.clear();
    }

    void LinearResampler::Process(const float *in,int count)
    {
        if(!in||count<=0)
            return;

        const double step=1.0/ratio;

        for(int i=0;i<count;i++)
        {
            if(has_last)
            {
                // 在区间 [last_in, in[i]] 内按 pos 产出
                while(pos<=1.0)
                {
                    const float frac=(float)pos;

                    out_buf.push_back(last_in*(1.0f-frac)+in[i]*frac);

                    pos+=step;
                }

                pos-=1.0;
            }
            else
            {
                out_buf.push_back(in[i]);

                pos=0.0;
            }

            last_in=in[i];
            has_last=true;
        }
    }

    int LinearResampler::ReadOutput(float *out,int max_count)
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
