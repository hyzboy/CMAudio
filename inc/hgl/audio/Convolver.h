#pragma once

#include<hgl/CoreType.h>
#include<hgl/audio/AudioFFT.h>
#include<hgl/audio/SplitComplex.h>
#include<vector>

namespace hgl::audio
{
    /**
    * 分段 FFT 卷积器（均匀块大小）
    *
    * 用脉冲响应初始化后，可对任意长度的输入流做实时卷积（无延迟，重叠相加法）。
    * Process() 期间无分配/锁/API 调用——适合实时音频线程。
    *
    * 移植自 Amplitude Audio SDK（Apache 2.0），
    * 原算法源自 HiFi-LoFi FFTConvolver（MIT）。
    */
    class Convolver
    {
        uint32 _blockSize=0;        ///< 分区大小（2 的幂）
        uint32 _segSize=0;          ///< 段大小 = 2×blockSize
        uint32 _segCount=0;         ///< IR 分段数
        uint32 _fftComplexSize=0;   ///< FFT 复数输出长度

        std::vector<SplitComplex*> _segments;       ///< 输入段频谱
        std::vector<SplitComplex*> _segmentsIR;     ///< IR 段频谱
        std::vector<float> _fftBuffer;
        AudioFFT _fft;
        SplitComplex _preMultiplied;
        SplitComplex _conv;
        std::vector<float> _overlap;
        uint32 _current=0;          ///< 当前输入段索引
        std::vector<float> _inputBuffer;
        uint32 _inputBufferFill=0;

    public:

        Convolver()=default;
        ~Convolver(){Reset();}

        Convolver(const Convolver &)=delete;
        Convolver &operator=(const Convolver &)=delete;

        /**
        * 初始化：指定分区大小与脉冲响应
        * @param block_size 分区大小（内部提升为 2 的幂）
        * @param ir 脉冲响应
        * @param ir_len IR 长度
        */
        bool Init(uint32 block_size,const float *ir,uint32 ir_len);

        /**
        * 卷积：输入 → 输出（长度 len，内部自动缓冲任意长度输入）
        */
        void Process(const float *input,float *output,uint32 len);

        void Reset();

        uint32 GetSegmentSize()const{return _segSize;}
        uint32 GetSegmentCount()const{return _segCount;}

    private:

        static uint32 NextPowerOf2(uint32 v);

        void CopyAndPad(std::vector<float> &dest,const float *src,uint32 src_size);

        void ComplexMultiplyAccumulate(float *re,float *im,
                                       const float *re_a,const float *im_a,
                                       const float *re_b,const float *im_b,uint32 len);

        void ComplexMultiplyAccumulate(SplitComplex &result,const SplitComplex &a,const SplitComplex &b);

        void Sum(float *result,const float *a,const float *b,uint32 len);
    };//class Convolver
}//namespace hgl::audio
