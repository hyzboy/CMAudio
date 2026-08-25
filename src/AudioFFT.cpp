#include<hgl/audio/AudioFFT.h>
#include<pffft/pffft.h>
#include<cstring>

namespace hgl::audio
{
    uint64 AudioFFT::GetOutputSize(uint64 input_size)
    {
        return input_size/2+1;
    }

    AudioFFT::AudioFFT()
    {
        impl=nullptr;
        size=0;
    }

    AudioFFT::~AudioFFT()
    {
        if(impl)
        {
            pffft_destroy_setup((PFFFT_Setup *)impl);
            impl=nullptr;
        }
    }

    void AudioFFT::Initialize(uint32 _size)
    {
        if(impl)
        {
            pffft_destroy_setup((PFFFT_Setup *)impl);
            impl=nullptr;
        }

        size=_size;

        if(size==0)
            return;

        impl=pffft_new_setup((int)size,PFFFT_REAL);
    }

    void AudioFFT::Forward(const float *input,SplitComplex &out)const
    {
        if(!impl)return;

        const uint32 complex_size=(uint32)GetOutputSize(size);

        out.Resize(complex_size);

        // 输入复制到对齐缓冲（PFFFT 要求 16/32 字节对齐）
        float *buffer=(float *)pffft_aligned_malloc(size*sizeof(float));
        float *scratch=(float *)pffft_aligned_malloc(size*sizeof(float));

        std::memcpy(buffer,input,size*sizeof(float));

        pffft_transform_ordered((PFFFT_Setup *)impl,buffer,buffer,scratch,PFFFT_FORWARD);

        // ordered 输出为交错 re/im（与 HiFi-LoFi AudioFFT 相同解包方式）
        float *re=out.re();
        float *im=out.im();

        const float *b=buffer;
        const float *b_end=buffer+size;

        while(b!=b_end)
        {
            *re++=*b++;
            *im++=*b++;
        }

        pffft_aligned_free(buffer);
        pffft_aligned_free(scratch);
    }

    void AudioFFT::Backward(float *output,SplitComplex &in)const
    {
        if(!impl)return;

        const uint32 complex_size=(uint32)GetOutputSize(size);

        in.Resize(complex_size);

        float *buffer=(float *)pffft_aligned_malloc(size*sizeof(float));
        float *scratch=(float *)pffft_aligned_malloc(size*sizeof(float));

        // 交错打包 re/im → buffer（只取前 N/2 对，Nyquist 分量忽略——与 HiFi-LoFi 原实现一致）
        float *b=buffer;
        float *b_end=buffer+size;
        const float *re=in.re();
        const float *im=in.im();

        while(b!=b_end)
        {
            *b++=*re++;
            *b++=*im++;
        }

        // PFFFT 逆变换不缩放（BACKWARD(FORWARD(x)) = N*x），需手动除以 N
        pffft_transform_ordered((PFFFT_Setup *)impl,buffer,output,scratch,PFFFT_BACKWARD);

        const float inv_n=1.0f/(float)size;

        for(uint32 i=0;i<size;i++)
            output[i]*=inv_n;

        pffft_aligned_free(buffer);
        pffft_aligned_free(scratch);
    }
}//namespace hgl::audio
