#pragma once

#include<hgl/CoreType.h>
#include<vector>

namespace hgl::audio
{
    /**
    * 流式线性插值重采样器（P1）
    *
    * 按 ratio（输出样本数/输入样本数）重采样，线性插值。
    * 与 WSOLAShifter 组合实现"变调不变速"（WSOLA 变速 + 本类重采样）。
    * 流式接口：Process 追加输入，ReadOutput 取走输出。
    */
    class LinearResampler
    {
        float sample_rate;
        float ratio;                    ///< 输出/输入 样本数比
        double pos;                     ///< 当前输入区间内位置（0..1）
        float last_in;                  ///< 上一输入样本
        bool  has_last;                 ///< 是否有上一样本
        std::vector<float> out_buf;     ///< 输出 FIFO

    public:

        LinearResampler();

        /**
        * 初始化
        * @param sample_rate 采样率（仅用于记录，重采样比率独立）
        * @param ratio 输出/输入 样本数比（0.5-2.0）
        */
        void Init(float sample_rate,float ratio=1.0f);

        void SetRatio(float r);         ///< 设置比率（0.5-2.0）
        float GetRatio()const{return ratio;}

        void Reset();

        void Process(const float *in,int count);
        int  GetOutputCount()const{return (int)out_buf.size();}
        int  ReadOutput(float *out,int max_count);
    };//class LinearResampler
}//namespace hgl::audio
