#pragma once

#include<hgl/audio/AudioFilterPreset.h>
#include<hgl/audio/AudioMixerTypes.h>

namespace hgl::audio
{
    /**
        * 音频源配置
        * 定义单个音频源的特性和生成参数
        */
    struct AudioMixerSourceConfig
    {
        const void* data;           ///< 音频数据指针(原始PCM数据起始地址)
        AudioDataInfo info;         ///< 音频数据信息（声道数、位深、是否浮点、采样率、数据大小）

        // 生成控制参数
        uint min_count;              ///< 最小生成数量(每类音源生成的实例下限)
        uint max_count;              ///< 最大生成数量(每类音源生成的实例上限)
        float min_interval;          ///< 最小出现间隔(秒)
        float max_interval;          ///< 最大出现间隔(秒)

        // 变化范围参数
        float min_volume;            ///< 最小音量(0.0-1.0，按实例随机)
        float max_volume;            ///< 最大音量(0.0-1.0，按实例随机)
        float min_pitch;             ///< 最小音调(0.5-2.0，按实例随机)
        float max_pitch;             ///< 最大音调(0.5-2.0，按实例随机)

        AudioFilterConfig filter_config; ///< 滤波参数(基准值，后续可叠加随机扰动)

        struct FilterRandomRange
        {
            float gain;    ///< 滤波整体增益随机范围(±gain)
            float gain_lf; ///< 低频增益随机范围(±gain_lf)
            float gain_hf; ///< 高频增益随机范围(±gain_hf)

            FilterRandomRange()
            {
                gain = 0.0f;
                gain_lf = 0.0f;
                gain_hf = 0.0f;
            }

            const bool operator ==(const FilterRandomRange& other) const
            {
                return gain == other.gain &&
                       gain_lf == other.gain_lf &&
                       gain_hf == other.gain_hf;
            }
        };

        struct SimpleReverbConfig
        {
            bool enable;        ///< 是否启用简易混响
            float delay_ms;     ///< 延迟时间(毫秒)
            float feedback;     ///< 反馈系数(0-0.95)
            float mix;          ///< 干湿比(0-1)

            float delay_ms_rand; ///< 延迟时间随机范围(±delay_ms_rand)
            float feedback_rand; ///< 反馈系数随机范围(±feedback_rand)
            float mix_rand;      ///< 干湿比随机范围(±mix_rand)

            SimpleReverbConfig()
            {
                enable = false;
                delay_ms = 80.0f;
                feedback = 0.3f;
                mix = 0.25f;
                delay_ms_rand = 0.0f;
                feedback_rand = 0.0f;
                mix_rand = 0.0f;
            }

            const bool operator ==(const SimpleReverbConfig& other) const
            {
                return enable == other.enable &&
                       delay_ms == other.delay_ms &&
                       feedback == other.feedback &&
                       mix == other.mix &&
                       delay_ms_rand == other.delay_ms_rand &&
                       feedback_rand == other.feedback_rand &&
                       mix_rand == other.mix_rand;
            }
        };

        FilterRandomRange filter_random;    ///< 滤波参数随机范围
        SimpleReverbConfig reverb;         ///< 简易混响参数

        AudioMixerSourceConfig()
        {
            data = nullptr;
            info = AudioDataInfo();

            min_count = 1;
            max_count = 1;
            min_interval = 0.0f;
            max_interval = 0.0f;

            min_volume = 0.8f;
            max_volume = 1.0f;
            min_pitch = 0.95f;
            max_pitch = 1.05f;

            filter_config = AudioFilterConfig();
        }

        const bool operator ==(const AudioMixerSourceConfig& cfg) const
        {
            return (data == cfg.data) &&
                   (info == cfg.info) &&
                   (min_count == cfg.min_count) && (max_count == cfg.max_count) &&
                   (min_interval == cfg.min_interval) && (max_interval == cfg.max_interval) &&
                   (min_volume == cfg.min_volume) && (max_volume == cfg.max_volume) &&
                   (min_pitch == cfg.min_pitch) && (max_pitch == cfg.max_pitch) &&
                   (filter_config == cfg.filter_config) &&
                   (filter_random == cfg.filter_random) &&
                   (reverb == cfg.reverb);
        }
    };//struct AudioMixerSourceConfig

    void ApplyAudioFilterPreset(AudioMixerSourceConfig &config, AudioFilterPreset preset);
}//namespace hgl::audio