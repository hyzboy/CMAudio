#pragma once

namespace hgl::audio
{
    /**
     * 音频滤波器类型
     * Audio filter type
     */
    enum class AudioFilterType
    {
        None=0,
        Lowpass,        ///< 低通
        Highpass,       ///< 高通
        Bandpass        ///< 带通
    };

    /**
     * 音频滤波器配置
     * Audio filter configuration
     */
    struct AudioFilterConfig
    {
        AudioFilterType type = AudioFilterType::None;
        float gain = 1.0f;       ///< 整体增益
        float gain_lf = 1.0f;    ///< 低频增益
        float gain_hf = 1.0f;    ///< 高频增益
        bool enable = true;      ///< 是否启用

        bool operator==(const AudioFilterConfig& other) const
        {
            return type == other.type &&
                   gain == other.gain &&
                   gain_lf == other.gain_lf &&
                   gain_hf == other.gain_hf &&
                   enable == other.enable;
        }
    };
}//namespace hgl::audio
