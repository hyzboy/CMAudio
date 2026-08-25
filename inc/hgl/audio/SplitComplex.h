#pragma once

#include<hgl/CoreType.h>
#include<complex>
#include<vector>

namespace hgl::audio
{
    /**
    * FFT 分裂复数缓冲（Split-Complex）
    *
    * 将 FFT 结果的实部/虚部分别存在两个连续缓冲中（利于 SIMD 优化）。
    * 移植自 Amplitude Audio SDK（Apache 2.0），原算法源自 HiFi-LoFi FFTConvolver（MIT）。
    */
    class SplitComplex
    {
    public:
        explicit SplitComplex(uint32 initial_size=0)
        {
            Resize(initial_size);
        }

        SplitComplex(const SplitComplex &)=delete;
        SplitComplex &operator=(const SplitComplex &)=delete;

        ~SplitComplex()=default;

        void Release()
        {
            _re.clear();
            _re.shrink_to_fit();
            _im.clear();
            _im.shrink_to_fit();
            _size=0;
        }

        void Resize(uint32 new_size,bool clear=false)
        {
            _re.resize(new_size);
            _im.resize(new_size);

            if(clear)
            {
                std::fill(_re.begin(),_re.end(),0.0f);
                std::fill(_im.begin(),_im.end(),0.0f);
            }

            _size=new_size;
        }

        void Clear()
        {
            std::fill(_re.begin(),_re.end(),0.0f);
            std::fill(_im.begin(),_im.end(),0.0f);
        }

        void CopyFrom(const SplitComplex &other)
        {
            std::copy(other._re.begin(),other._re.end(),_re.begin());
            std::copy(other._im.begin(),other._im.end(),_im.begin());
        }

        uint32 GetSize()const{return _size;}

        float *re(){return _re.data();}
        const float *re()const{return _re.data();}

        float *im(){return _im.data();}
        const float *im()const{return _im.data();}

        std::complex<float> operator[](uint32 index)const
        {
            return std::complex<float>(_re[index],_im[index]);
        }

    private:
        uint32 _size=0;
        std::vector<float> _re;
        std::vector<float> _im;
    };//class SplitComplex
}//namespace hgl::audio
