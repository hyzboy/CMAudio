#pragma once

#include<hgl/CoreType.h>
#include<hgl/audio/SplitComplex.h>

namespace hgl::audio
{
    /**
    * 实信号 FFT（包装 PFFFT）
    *
    * 正变换：实数输入 → 分裂复数（实/虚分离）；逆变换：分裂复数 → 实数输出（自带 1/N 缩放）。
    * 底层 PFFFT 移植自 HiFi-LoFi AudioFFT（MIT），算法源自 FFTPACK。
    * 移植自 Amplitude Audio SDK（Apache 2.0）。
    *
    * 注意：与 AudioAnalysis.h 的自由函数 FFT() 不同名（该类为 AudioFFT）。
    */
    class AudioFFT
    {
        void *impl=nullptr;     ///< PFFFT_Setup
        uint32 size=0;          ///< FFT 大小（2 的幂）

    public:

        static uint64 GetOutputSize(uint64 input_size);     ///< 分裂复数缓冲长度（= input_size/2+1）

        AudioFFT();
        ~AudioFFT();

        AudioFFT(const AudioFFT &)=delete;
        AudioFFT &operator=(const AudioFFT &)=delete;

        void Initialize(uint32 size);                       ///< 初始化（size 必须为 2 的幂）
        void Forward(const float *input,SplitComplex &out)const;    ///< 正变换（input 长度=size）
        void Backward(float *output,SplitComplex &in)const;         ///< 逆变换（output 长度=size，含 1/N 缩放）
    };//class AudioFFT
}//namespace hgl::audio
