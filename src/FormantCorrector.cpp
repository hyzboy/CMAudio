#include<hgl/audio/FormantCorrector.h>
#include<hgl/audio/AudioAnalysis.h>
#include<cmath>

namespace hgl::audio
{
    namespace
    {
        constexpr float FC_PI = 3.14159265358979323846f;

        void FC_IFFT(float *real,float *imag,int n)
        {
            for(int i=0;i<n;i++)
                imag[i]=-imag[i];

            FFT(real,imag,n);

            for(int i=0;i<n;i++)
            {
                imag[i]=-imag[i];
                real[i]/=float(n);
            }
        }

        // cepstral 包络：输入帧频谱 → log 幅度 → IFFT → lifter → FFT → 包络
        void ComputeEnvelope(const float *frame,int N,int half,int n_cep,
                             const float *real_in,const float *imag_in,
                             std::vector<float> &env,
                             std::vector<float> &cep_real,std::vector<float> &cep_imag)
        {
            for(int k=0;k<=half;k++)
                env[k]=std::log(std::sqrt(real_in[k]*real_in[k]+imag_in[k]*imag_in[k])+1.0e-9f);

            for(int k=0;k<=half;k++)
            {
                cep_real[k]=env[k];
                cep_imag[k]=0.0f;
            }

            for(int k=1;k<half;k++)
            {
                cep_real[N-k]=env[k];
                cep_imag[N-k]=0.0f;
            }

            FC_IFFT(cep_real.data(),cep_imag.data(),N);

            for(int k=n_cep;k<N;k++)
            {
                cep_real[k]=0.0f;
                cep_imag[k]=0.0f;
            }

            FFT(cep_real.data(),cep_imag.data(),N);

            for(int k=0;k<=half;k++)
                env[k]=cep_real[k];
        }
    }//namespace

    FormantCorrector::FormantCorrector()
    {
    }

    bool FormantCorrector::Process(const float *in,const float *shifted,int count,float sample_rate,
                                   std::vector<float> &out)
    {
        out.clear();

        if(!in||!shifted||count<=0||sample_rate<=0)
            return false;

        const int N=2048;
        const int half=N/2;
        const int hop=N/4;
        const int n_cep=128;

        if(count<N)
        {
            // 太短：直通（无法校正）
            out.assign(shifted,shifted+count);
            return true;
        }

        // sqrt-Hann 窗
        std::vector<float> win(N);

        for(int i=0;i<N;i++)
            win[i]=std::sqrt(0.5f-0.5f*std::cos(2.0f*FC_PI*(float)i/(float)(N-1)));

        const int n_frames=(count-N)/hop+1;

        std::vector<float> r_in(N),i_in(N);
        std::vector<float> r_sh(N),i_sh(N);
        std::vector<float> r_out(N),i_out(N);
        std::vector<float> env_in(half+1),env_sh(half+1);
        std::vector<float> cep_r(N),cep_i(N);

        out.assign(count+N,0.0f);

        int pos_s=0;

        for(int f=0;f<n_frames;f++)
        {
            const int pos=f*hop;

            // 分析原信号帧
            for(int k=0;k<N;k++)
            {
                r_in[k]=in[pos+k]*win[k];
                i_in[k]=0.0f;
            }

            FFT(r_in.data(),i_in.data(),N);
            ComputeEnvelope(nullptr,N,half,n_cep,r_in.data(),i_in.data(),env_in,cep_r,cep_i);

            // 分析变调输出帧
            for(int k=0;k<N;k++)
            {
                r_sh[k]=shifted[pos+k]*win[k];
                i_sh[k]=0.0f;
            }

            FFT(r_sh.data(),i_sh.data(),N);
            ComputeEnvelope(nullptr,N,half,n_cep,r_sh.data(),i_sh.data(),env_sh,cep_r,cep_i);

            // 幅度整形：Y'' = Y' × clamp(env_in/env_sh)
            for(int k=0;k<=half;k++)
            {
                const float ratio=std::exp(env_in[k]-env_sh[k]);

                float g=ratio;

                if(g>20.0f)g=20.0f;
                if(g<0.05f)g=0.05f;

                r_out[k]=r_sh[k]*g;
                i_out[k]=i_sh[k]*g;
            }

            for(int k=1;k<half;k++)
            {
                r_out[N-k]=r_out[k];
                i_out[N-k]=-i_out[k];
            }

            i_out[0]=0.0f;
            i_out[half]=0.0f;

            FC_IFFT(r_out.data(),i_out.data(),N);

            for(int k=0;k<N;k++)
                out[pos_s+k]+=r_out[k]*win[k]*0.5f;

            pos_s+=hop;
        }

        out.resize(count);

        return true;
    }
}//namespace hgl::audio
