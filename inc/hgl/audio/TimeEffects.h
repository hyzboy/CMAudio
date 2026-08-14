#pragma once

#include<hgl/CoreType.h>
#include<vector>
#include<cmath>

namespace hgl::audio
{
    namespace
    {
        constexpr float PI_F = 3.14159265358979323846f;
    }

    /**
    * 环形缓冲延迟线（P4 时域效果的基础）
    *   Write 写入样本，Read 读取 delay 个样本之前的样本（delay=0 即刚写入的）。
    *   ReadInterpolated 支持分数延迟（线性插值，用于 Chorus/Flanger 的 LFO 调制）。
    *   header-only（DSP 热点，inline 便于优化）。
    */
    class DelayLine
    {
        std::vector<float> buffer;
        int size;
        int write_index;    ///< 指向最后写入的位置

    public:
        DelayLine()
        {
            size=0;
            write_index=0;
        }

        void Init(int max_delay_samples)        ///< 分配缓冲（至少 delay+1）
        {
            size = (max_delay_samples > 0) ? max_delay_samples : 1;
            buffer.assign(size, 0.0f);
            write_index = size - 1;             // 首次 Write 前进到 0
        }

        void Reset()
        {
            if(size > 0)
            {
                buffer.assign(size, 0.0f);
                write_index = size - 1;
            }
        }

        void Write(float x)
        {
            write_index = (write_index + 1) % size;
            buffer[write_index] = x;
        }

        float Read(float delay_samples)         ///< 整数延迟读取（delay=0 即刚写入的）
        {
            int pos = write_index - (int)delay_samples;

            while(pos < 0)
                pos += size;

            return buffer[pos % size];
        }

        float ReadInterpolated(float delay_samples)   ///< 线性插值读取（支持分数延迟）
        {
            float pos = (float)write_index - delay_samples;

            while(pos < 0.0f)
                pos += (float)size;

            const int i0 = (int)pos % size;
            const int i1 = (i0 + 1) % size;
            const float frac = pos - (float)((int)pos);

            return buffer[i0] * (1.0f - frac) + buffer[i1] * frac;
        }
    };//class DelayLine

    /**
    * 回声（Echo）：延迟 + 反馈
    *   feedback=0 且 mix=1 时是纯延迟；feedback>0 产生衰减的多次回声。
    */
    class Echo
    {
        DelayLine line;
        float sample_rate;
        float delay_samples;
        float feedback;     ///< 反馈系数（0..1）
        float mix;          ///< 干湿比（0=全干，1=全湿）

    public:
        Echo()
        {
            sample_rate=48000.0f;
            delay_samples=0.0f;
            feedback=0.0f;
            mix=0.5f;
        }

        void Init(float sr, float delay_sec, float fb=0.0f, float m=0.5f)
        {
            sample_rate=sr;
            delay_samples=delay_sec*sr;
            if(delay_samples<1.0f)delay_samples=1.0f;   // 最小 1 样本延迟
            feedback=fb;
            mix=m;

            line.Init((int)delay_samples+1);
        }

        void Reset(){ line.Reset(); }

        float Process(float x)
        {
            // 先 Read 再 Write：Read 时 write_index 指向下一个写入位置，
            // 故 D 样本延迟需读 D-1 个样本之前（off-by-one 修正）。
            const float delayed=line.Read(delay_samples-1.0f);

            line.Write(x+delayed*feedback);

            return x*(1.0f-mix)+delayed*mix;
        }

        void Process(float *samples,int count)
        {
            if(!samples)return;

            for(int i=0;i<count;i++)
                samples[i]=Process(samples[i]);
        }

        float GetDelaySeconds()const{return delay_samples/sample_rate;}
        float GetFeedback()const{return feedback;}
        float GetMix()const{return mix;}
    };//class Echo

    /**
    * 合唱/镶边（Chorus/Flanger）：LFO 调制延迟时间
    *   Chorus：较长延迟（~20ms）+ 慢 LFO + 少反馈 → 多声部合唱感
    *   Flanger：短延迟（~5ms）+ 快 LFO + 强反馈 → 扫频梳状滤波
    *   两者本质相同，仅参数不同。
    */
    class Chorus
    {
        DelayLine line;
        float sample_rate;
        float base_delay_sec;   ///< 基础延迟
        float lfo_rate_hz;      ///< LFO 频率
        float lfo_depth_sec;    ///< LFO 深度（延迟调制幅度）
        float feedback;         ///< 反馈系数
        float mix;              ///< 干湿比
        float lfo_phase;        ///< LFO 相位（0..1）

    public:
        Chorus()
        {
            sample_rate=48000.0f;
            base_delay_sec=0.020f;
            lfo_rate_hz=0.5f;
            lfo_depth_sec=0.005f;
            feedback=0.0f;
            mix=0.5f;
            lfo_phase=0.0f;
        }

        void Init(float sr,float base_delay,float lfo_rate,float lfo_depth,float fb=0.0f,float m=0.5f)
        {
            sample_rate=sr;
            base_delay_sec=base_delay;
            lfo_rate_hz=lfo_rate;
            lfo_depth_sec=lfo_depth;
            feedback=fb;
            mix=m;
            lfo_phase=0.0f;

            line.Init((int)((base_delay+lfo_depth)*sr)+2);
        }

        void Reset()
        {
            line.Reset();
            lfo_phase=0.0f;
        }

        float Process(float x)
        {
            const float lfo=std::sin(2.0f*PI_F*lfo_phase);
            const float delay=(base_delay_sec+lfo*lfo_depth_sec)*sample_rate;
            const float delayed=line.ReadInterpolated(delay);

            line.Write(x+delayed*feedback);

            lfo_phase+=lfo_rate_hz/sample_rate;
            if(lfo_phase>=1.0f)
                lfo_phase-=1.0f;

            return x*(1.0f-mix)+delayed*mix;
        }

        void Process(float *samples,int count)
        {
            if(!samples)return;

            for(int i=0;i<count;i++)
                samples[i]=Process(samples[i]);
        }

        // 典型参数便捷工厂
        static Chorus CreateChorus(float sr)
        {
            Chorus c;
            c.Init(sr,0.020f,0.5f,0.005f,0.0f,0.5f);     // 20ms 基础延迟 + 0.5Hz LFO ±5ms
            return c;
        }

        static Chorus CreateFlanger(float sr)
        {
            Chorus c;
            c.Init(sr,0.005f,0.2f,0.003f,0.5f,0.5f);     // 5ms 基础延迟 + 0.2Hz LFO ±3ms + 反馈
            return c;
        }
    };//class Chorus
}//namespace hgl::audio
