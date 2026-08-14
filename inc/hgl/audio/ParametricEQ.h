#pragma once

#include<hgl/CoreType.h>
#include<hgl/audio/BiquadFilter.h>
#include<vector>

namespace hgl::audio
{
    /**
    * 参数化均衡器（PEQ，P2）：N 段双二阶滤波器级联
    *   每段 = 一个 BiquadFilter（Peaking/Shelf/HP/LP/Notch/Bandpass）
    *
    * 典型用法：
    *   ParametricEQ eq(48000);
    *   eq.AddBand(BiquadType::LowShelf, 100.0f, 0.7071f, +6.0f);  // 低频抬升
    *   eq.AddBand(BiquadType::Peaking,  1000.0f, 1.0f,   0.0f);   // 中频平坦
    *   eq.AddBand(BiquadType::HighShelf,10000.0f,0.7071f, -6.0f); // 高频衰减
    *   每样本 eq.Process(x)，或批量 eq.Process(buf, count);
    *
    * 级联顺序 = 添加顺序（线性时不变系统，级联可交换，顺序不改变幅度响应）。
    */
    class ParametricEQ
    {
    public:
        struct Band
        {
            BiquadType type;        ///< 滤波器类型
            float frequency;        ///< 中心/截止频率（Hz）
            float q;                ///< Q 值
            float gain_db;          ///< 增益（dB，仅 Peaking/Shelf 生效）
        };

    private:
        float sample_rate;
        std::vector<Band> bands;
        std::vector<BiquadFilter> stages;   ///< 与 bands 一一对应的级联段

    public:
        ParametricEQ();
        ParametricEQ(float sample_rate);

        void SetSampleRate(float sample_rate);  ///< 重设采样率（重算所有段系数）
        float GetSampleRate()const{return sample_rate;}

        int  AddBand(BiquadType type, float frequency, float q = 0.7071f, float gain_db = 0.0f);  ///< 添加段，返回索引
        bool SetBand(int index, BiquadType type, float frequency, float q, float gain_db);        ///< 修改段参数
        bool RemoveBand(int index);                                                               ///< 删除段
        void ClearBands();                                                                        ///< 清空所有段

        int GetBandCount()const{return (int)bands.size();}
        const Band *GetBand(int index)const;                                                      ///< 获取段参数（越界返回 nullptr）

        void  Reset();                                      ///< 清零所有段状态
        float Process(float x);                             ///< 单样本处理
        void  Process(float *samples, int count);           ///< 批量原地处理

        /**
        * 便捷：3-band EQ（低频 shelf + 中频峰值 + 高频 shelf）
        */
        static ParametricEQ Create3Band(float sample_rate,
                                        float low_freq,  float low_gain_db,
                                        float mid_freq,  float mid_gain_db, float mid_q,
                                        float high_freq, float high_gain_db);
    };//class ParametricEQ
}//namespace hgl::audio
