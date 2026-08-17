#pragma once

#include<hgl/CoreType.h>
#include<hgl/audio/OfflinePitchShift.h>
#include<vector>

namespace hgl::audio
{
    /**
    * 离线高质量变声器（P2）
    *
    * 管线：OfflinePitchShift（WSOLA + SINC 变调）→ FormantCorrector（formant 保持）
    *       → 可选 EQ（高频提亮）→ 可选响度归一化。
    * 用于离线处理（录音/文件 → 变声 → 导出），质量优先、无实时约束。
    */
    class OfflineVoiceFX
    {
    public:

        struct Settings
        {
            float pitch=1.0f;               ///< 变调比（0.5-2.0，2.0=升八度）
            bool  preserve_formants=true;   ///< formant 保持（语音变声推荐 true）
            float high_shelf_db=0.0f;       ///< 高频提亮（dB，0=关）
            float target_lufs=0.0f;         ///< 目标响度 LUFS（0=不归一化）
        };//struct Settings

        /**
        * 离线处理 float mono PCM
        * @param in 输入样本
        * @param count 输入样本数
        * @param sample_rate 采样率
        * @param s 处理设置
        * @param out 输出
        * @return 是否成功
        */
        bool Process(const float *in,int count,float sample_rate,
                     const Settings &s,std::vector<float> &out);
    };//class OfflineVoiceFX
}//namespace hgl::audio
