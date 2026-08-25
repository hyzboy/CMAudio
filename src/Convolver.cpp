#include<hgl/audio/Convolver.h>
#include<cmath>
#include<cstring>
#include<algorithm>

namespace hgl::audio
{
    uint32 Convolver::NextPowerOf2(uint32 v)
    {
        if(v<=1)return 1;

        uint32 n=1;

        while(n<v)
            n<<=1;

        return n;
    }

    void Convolver::CopyAndPad(std::vector<float> &dest,const float *src,uint32 src_size)
    {
        std::memcpy(dest.data(),src,src_size*sizeof(float));
        std::memset(dest.data()+src_size,0,(dest.size()-src_size)*sizeof(float));
    }

    void Convolver::ComplexMultiplyAccumulate(float *re,float *im,
                                              const float *re_a,const float *im_a,
                                              const float *re_b,const float *im_b,uint32 len)
    {
        if(len==0)return;

        // bin0 特例：PFFFT ordered 布局把 (DC, Nyquist) 打包在第一个复数
        // （re[0]=DC 实部, im[0]=Nyquist 实部）——两者都是实数 bin，必须实乘：
        //   F(0)   = F(0)_A * F(0)_B
        //   F(N/2) = F(N/2)_A * F(N/2)_B
        // 标准复数乘法会让 DC 与 Nyquist 交叉污染（卷积错误）
        re[0]+=re_a[0]*re_b[0];
        im[0]+=im_a[0]*im_b[0];

        // bin 1..len-1：标准复数乘法
        for(uint32 i=1;i<len;i++)
        {
            const float r=(re_a[i]*re_b[i])-(im_a[i]*im_b[i]);
            const float ii=(re_a[i]*im_b[i])+(im_a[i]*re_b[i]);

            re[i]+=r;
            im[i]+=ii;
        }
    }

    void Convolver::ComplexMultiplyAccumulate(SplitComplex &result,const SplitComplex &a,const SplitComplex &b)
    {
        ComplexMultiplyAccumulate(result.re(),result.im(),a.re(),a.im(),b.re(),b.im(),result.GetSize());
    }

    void Convolver::Sum(float *result,const float *a,const float *b,uint32 len)
    {
        for(uint32 i=0;i<len;i++)
            result[i]=a[i]+b[i];
    }

    void Convolver::Reset()
    {
        for(uint32 i=0;i<_segCount;i++)
        {
            delete _segments[i];
            delete _segmentsIR[i];
        }

        _blockSize=0;
        _segSize=0;
        _segCount=0;
        _fftComplexSize=0;

        _segments.clear();
        _segmentsIR.clear();
        _fftBuffer.clear();
        _fft.Initialize(0);
        _preMultiplied.Release();
        _conv.Release();
        _overlap.clear();
        _current=0;
        _inputBuffer.clear();
        _inputBufferFill=0;
    }

    bool Convolver::Init(uint32 block_size,const float *ir,uint32 ir_len)
    {
        Reset();

        if(block_size==0)
            return false;

        // 忽略 IR 尾部接近零的部分（省计算）
        while(ir_len>0&&std::fabs(ir[ir_len-1])<0.000001f)
            --ir_len;

        if(ir_len==0)
            return true;

        _blockSize=NextPowerOf2(block_size);
        _segSize=2*_blockSize;
        _segCount=(uint32)std::ceil((float)ir_len/(float)_blockSize);
        _fftComplexSize=(uint32)AudioFFT::GetOutputSize(_segSize);

        // FFT
        _fft.Initialize(_segSize);
        _fftBuffer.resize(_segSize);

        // 输入段
        for(uint32 i=0;i<_segCount;i++)
            _segments.push_back(new SplitComplex(_fftComplexSize));

        // IR 段
        for(uint32 i=0;i<_segCount;i++)
        {
            SplitComplex *segment=new SplitComplex(_fftComplexSize);

            const uint32 remaining=ir_len-(i*_blockSize);
            const uint32 size_copy=(remaining>=_blockSize)?_blockSize:remaining;

            CopyAndPad(_fftBuffer,&ir[i*_blockSize],size_copy);
            _fft.Forward(_fftBuffer.data(),*segment);

            _segmentsIR.push_back(segment);
        }

        // 卷积缓冲
        _preMultiplied.Resize(_fftComplexSize,true);
        _conv.Resize(_fftComplexSize,true);
        _overlap.resize(_blockSize);

        // 输入缓冲
        _inputBuffer.resize(_blockSize);
        _inputBufferFill=0;

        // 当前位置
        _current=0;

        return true;
    }

    void Convolver::Process(const float *input,float *output,uint32 len)
    {
        if(_segCount==0)
        {
            std::memset(output,0,len*sizeof(float));
            return;
        }

        uint32 processed=0;

        while(processed<len)
        {
            const bool input_buffer_was_empty=(_inputBufferFill==0);
            const uint32 processing=std::min(len-processed,_blockSize-_inputBufferFill);
            const uint32 input_buffer_pos=_inputBufferFill;

            std::memcpy(_inputBuffer.data()+input_buffer_pos,input+processed,processing*sizeof(float));

            // 正变换：Forward 整个 blockSize 块（HiFi-LoFi 原版语义——
            // 复制完整 _inputBuffer 而非 processing 个；Amplitude 改 processing 是其 bug）
            CopyAndPad(_fftBuffer,_inputBuffer.data(),_blockSize);
            _fft.Forward(_fftBuffer.data(),*_segments[_current]);

            // 复数乘法累加
            if(input_buffer_was_empty)
            {
                _preMultiplied.Clear();

                for(uint32 i=1;i<_segCount;i++)
                {
                    const uint32 index_ir=i;
                    const uint32 index_audio=(_current+i)%_segCount;

                    ComplexMultiplyAccumulate(_preMultiplied,*_segmentsIR[index_ir],*_segments[index_audio]);
                }
            }

            _conv.CopyFrom(_preMultiplied);
            ComplexMultiplyAccumulate(_conv,*_segmentsIR[0],*_segments[_current]);

            // 逆变换
            _fft.Backward(_fftBuffer.data(),_conv);

            // 重叠相加
            Sum(output+processed,_fftBuffer.data()+input_buffer_pos,_overlap.data()+input_buffer_pos,processing);

            // 输入缓冲满 → 下一块
            _inputBufferFill+=processing;

            if(_inputBufferFill==_blockSize)
            {
                // 输入缓冲清空
                std::memset(_inputBuffer.data(),0,_blockSize*sizeof(float));
                _inputBufferFill=0;

                // 保存重叠
                std::memcpy(_overlap.data(),_fftBuffer.data()+_blockSize,_blockSize*sizeof(float));

                // 更新当前段
                _current=(_current>0)?(_current-1):(_segCount-1);
            }

            processed+=processing;
        }
    }
}//namespace hgl::audio
