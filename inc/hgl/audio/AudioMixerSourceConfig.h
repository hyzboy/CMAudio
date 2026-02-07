#pragma once

#include<hgl/audio/AudioFilterPreset.h>

namespace hgl::audio
{
    /**
        * 音频源配置
        * 定义单个音频源的特性和生成参数
        */
    struct AudioMixerSourceConfig
    {
        const void* data;           ///< 音频数据指针(原始PCM数据起始地址)
        uint dataSize;              ///< 数据大小(字节数)
        uint format;                ///< 音频格式(如AL_FORMAT_MONO16/AL_FORMAT_MONO8)
        uint sampleRate;            ///< 采样率(Hz，与输出一致)

        // 生成控制参数
        uint minCount;              ///< 最小生成数量(每类音源生成的实例下限)
        uint maxCount;              ///< 最大生成数量(每类音源生成的实例上限)
        float minInterval;          ///< 最小出现间隔(秒)
        float maxInterval;          ///< 最大出现间隔(秒)

        // 变化范围参数
        float minVolume;            ///< 最小音量(0.0-1.0，按实例随机)
        float maxVolume;            ///< 最大音量(0.0-1.0，按实例随机)
        float minPitch;             ///< 最小音调(0.5-2.0，按实例随机)
        float maxPitch;             ///< 最大音调(0.5-2.0，按实例随机)

        AudioFilterConfig filterConfig; ///< 滤波参数(基准值，后续可叠加随机扰动)

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

        FilterRandomRange filterRandom;    ///< 滤波参数随机范围
        SimpleReverbConfig reverb;         ///< 简易混响参数

        AudioMixerSourceConfig()
        {
            data = nullptr;
            dataSize = 0;
            format = 0;
            sampleRate = 0;

            minCount = 1;
            maxCount = 1;
            minInterval = 0.0f;
            maxInterval = 0.0f;

            minVolume = 0.8f;
            maxVolume = 1.0f;
            minPitch = 0.95f;
            maxPitch = 1.05f;

            filterConfig = AudioFilterConfig();
        }

        const bool operator ==(const AudioMixerSourceConfig& cfg) const
        {
            return (data == cfg.data) && (dataSize == cfg.dataSize) &&
                   (format == cfg.format) && (sampleRate == cfg.sampleRate) &&
                   (minCount == cfg.minCount) && (maxCount == cfg.maxCount) &&
                   (minInterval == cfg.minInterval) && (maxInterval == cfg.maxInterval) &&
                   (minVolume == cfg.minVolume) && (maxVolume == cfg.maxVolume) &&
                     (minPitch == cfg.minPitch) && (maxPitch == cfg.maxPitch) &&
                                         (filterConfig.type == cfg.filterConfig.type) &&
                                         (filterConfig.gain == cfg.filterConfig.gain) &&
                                         (filterConfig.gain_lf == cfg.filterConfig.gain_lf) &&
                                         (filterConfig.gain_hf == cfg.filterConfig.gain_hf) &&
                                         (filterConfig.enable == cfg.filterConfig.enable) &&
                                         (filterRandom == cfg.filterRandom) &&
                                         (reverb == cfg.reverb);
        }
    };//struct AudioMixerSourceConfig

    void ApplyAudioFilterPreset(AudioMixerSourceConfig &config, AudioFilterPreset preset);
}//namespace hgl::audio