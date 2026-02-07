#pragma once

#include<hgl/audio/AudioMixer.h>
#include<hgl/audio/AudioMixerSourceConfig.h>
#include<hgl/audio/AudioMemoryPool.h>
#include<hgl/type/UnorderedMap.h>
#include<hgl/log/Log.h>
#include<random>

namespace hgl::audio
{
    /**
        * 音频场景混音器
        * 高级混音控制器，用于管理多种音源的混合
        * 可以控制每种音源的数量、出现频率、变化幅度等
        */
    class AudioMixerScene
    {
        OBJECT_LOGGER

    private:

        UnorderedMap<OSString,AudioMixerSourceConfig> sources;    ///< 音频源字典
        MixerConfig globalConfig;               ///< 全局混音配置

        uint sourceFormat;                      ///< 所有音源的统一格式(首个添加的音源决定)
        uint sourceSampleRate;                  ///< 所有音源的统一采样率(首个添加的音源决定)
        uint outputFormat;                      ///< 输出音频格式
        uint outputSampleRate;                  ///< 输出采样率

        std::random_device rd;
        std::mt19937 rng;

        // 内存池 - 避免频繁分配/释放
        AudioMemoryPool<char> poolBuffer;       ///< 输出场景缓冲池
        AudioMemoryPool<char> tempBuffer;       ///< 临时实例混音缓冲池

        /**
            * 生成随机浮点数
            */
        float RandomFloat(float min, float max);

        /**
            * 生成随机整数
            */
        uint RandomUInt(uint min, uint max);

        bool HasAnyEffects() const;
        AudioFilterConfig BuildRandomFilterConfig(const AudioMixerSourceConfig& config);
        void ApplyLowpass(float* samples, uint count, float alpha);
        void ApplyHighpass(float* samples, uint count, float alpha);
        void ApplyFilter(float* samples, uint count, const AudioFilterConfig& config);
        void ApplySimpleReverb(float* samples, uint count, uint sampleRate, const AudioMixerSourceConfig::SimpleReverbConfig& config);
        bool ConvertFloatToOutput(const float* input, uint sampleCount, void** outputData, uint* outputSize);

    public:

        AudioMixerScene();
        virtual ~AudioMixerScene();

        /**
            * 添加音频源
            * @param name 音频源名称(如"小轿车"、"SUV"、"喇叭"等)
            * @param config 音频源配置
            */
        void AddSource(const OSString& name, const AudioMixerSourceConfig& config);

        /**
            * 移除音频源
            */
        void RemoveSource(const OSString& name);

        /**
            * 清除所有音频源
            */
        void ClearSources();

        /**
            * 获取音频源数量
            */
        int GetSourceCount() const { return sources.GetCount(); }

        /**
            * 设置全局混音配置
            */
        void SetGlobalConfig(const MixerConfig& cfg) { globalConfig = cfg; }

        /**
            * 获取全局混音配置
            */
        const MixerConfig& GetGlobalConfig() const { return globalConfig; }

        /**
            * 设置输出格式
            * @param format 输出音频格式 (AL_FORMAT_MONO8/MONO16)
            * @param sampleRate 输出采样率 (如44100, 48000)
            */
        void SetOutputFormat(uint format, uint sampleRate);

        /**
            * 获取输出格式
            */
        uint GetOutputFormat() const { return outputFormat; }

        /**
            * 获取输出采样率
            */
        uint GetOutputSampleRate() const { return outputSampleRate; }

        /**
            * 生成混音场景
            * @param outputData 输出数据指针(需要调用者释放)
            * @param outputSize 输出数据大小
            * @param duration 场景持续时间(秒)
            * @return 是否成功
            */
        bool GenerateScene(void** outputData, uint* outputSize, float duration);
    };
}//namespace hgl::audio
