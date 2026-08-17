#pragma once

#include<hgl/CoreType.h>
#include<vector>

namespace hgl::audio
{
    /**
    * formant 保持校正（P2，离线）
    *
    * 变调会把 formant 一起搬移（频率 ×pitch）。本类把 formant 包络搬回原位：
    *   逐帧 STFT，cepstral liftering 分离包络（env_in 原信号 / env_out 变调输出），
    *   幅度整形 Y''[k] = Y'[k] × env_in[k]/env_out[k]（比值 clamp 防放大噪声），
    *   相位保持 → 基频已变调、formant 留在原频率（"说话人声道"不变）。
    *
    * 要求：in（原信号）与 shifted（变调输出）长度一致（同一帧网格）。
    */
    class FormantCorrector
    {
    public:

        FormantCorrector();

        /**
        * formant 校正
        * @param in 原信号（float mono）
        * @param shifted 变调后的信号（长度应与 in 一致）
        * @param count 样本数
        * @param sample_rate 采样率
        * @param out 校正输出
        * @return 是否成功
        */
        bool Process(const float *in,const float *shifted,int count,float sample_rate,
                     std::vector<float> &out);
    };//class FormantCorrector
}//namespace hgl::audio
