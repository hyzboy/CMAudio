#pragma once

#include<hgl/CoreType.h>
#include<vector>

namespace hgl::audio
{
    /**
    * 波形相似重叠相加（WSOLA）变速器（P1）
    *
    * 变速不变调（time-stretch）：把输入信号时长变为 stretch 倍，音调保持不变。
    * SoundTouch 式结构：
    *   - 输出帧 = 窗口 W（win_ms），前 overlap（W/2）与 pMidBuffer 等功率 crossfade
    *   - pMidBuffer = 当前窗口之后的 W 样本（下一帧的衔接参考）
    *   - 输入锚点每帧前进 delta_in = W/stretch，在 ±search 内搜索与 pMidBuffer
    *     前 overlap 归一化互相关最强的片段，实现无缝拼接
    *
    * 输出/输入 样本比 = W/delta_in = stretch。
    * 用途：与 LinearResampler 组合成"变调不变速"的实时 PitchShifter（变声器核心）。
    *
    * 流式接口：Process 追加输入，ReadOutput 取走输出。
    */
    class WSOLAShifter
    {
        float sample_rate;
        float stretch;                  ///< 输出时长/输入时长（0.5-2.0）

        int   W;                        ///< 窗口长（输出帧长，样本）
        int   overlap;                  ///< crossfade 长（= W/2）
        int   delta_in;                 ///< 输入锚点前进量（= W/stretch）
        int   search;                   ///< 搜索半宽（样本）

        std::vector<float> in_buf;      ///< 输入 FIFO
        std::vector<float> out_buf;     ///< 输出 FIFO
        std::vector<float> mid_buf;     ///< pMidBuffer（上一窗口后的 W 样本）
        int   in_anchor;                ///< 下一帧输入锚点（in_buf 索引）
        bool  first_frame;              ///< 是否首帧（跳过搜索）

        int   NeededInput()const{return in_anchor+delta_in+search+2*W;}
        int   FindBest(int lo,int hi);  ///< 与 mid_buf 前 overlap 的归一化互相关搜索
        void  ProduceFrame();           ///< 产出一帧（W 样本 → out_buf）

    public:

        WSOLAShifter();

        /**
        * 初始化
        * @param sample_rate 采样率
        * @param stretch_ratio 输出时长/输入时长（0.5-2.0）
        * @param win_ms 窗口长（毫秒，默认 40；输出帧长 = 窗口长）
        * @param search_ms 搜索半宽（毫秒，默认 10）
        */
        void Init(float sample_rate,float stretch_ratio=1.0f,float win_ms=40.0f,float search_ms=10.0f);

        void SetStretch(float ratio);   ///< 设置变速比（0.5-2.0）
        float GetStretch()const{return stretch;}

        void Reset();                   ///< 清空所有状态

        void Process(const float *in,int count);    ///< 追加输入
        int  GetOutputCount()const{return (int)out_buf.size();}
        int  ReadOutput(float *out,int max_count);  ///< 取走输出，返回实际样本数
    };//class WSOLAShifter
}//namespace hgl::audio
