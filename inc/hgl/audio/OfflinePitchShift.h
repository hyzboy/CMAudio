#pragma once

#include<hgl/CoreType.h>
#include<vector>

namespace hgl::audio
{
    /**
    * 离线高质量变调（P2）：变调不变速
    *
    * SoundTouch 级组合（工业验证）：
    *   WSOLAShifter（大窗 60ms 高质量变速，stretch=pitch，频率不变）
    *   → libsamplerate SINC 重采样（ratio=1/pitch，频率 ×pitch、时长恢复）
    *
    * 离线整段处理，质量优先（无实时约束）。
    */
    class OfflinePitchShift
    {
    public:

        OfflinePitchShift();

        /**
        * 离线变调（float mono）
        * @param in 输入样本
        * @param in_count 输入样本数
        * @param pitch 变调比（0.5-2.0）
        * @param sample_rate 采样率
        * @param out 输出（时长 ≈ 输入，已对齐 in_count）
        * @return 是否成功
        */
        bool Process(const float *in,int in_count,float pitch,float sample_rate,
                     std::vector<float> &out);
    };//class OfflinePitchShift
}//namespace hgl::audio
