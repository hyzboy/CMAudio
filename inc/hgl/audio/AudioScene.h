#pragma once

#include<hgl/audio/AudioMixer.h>
#include<hgl/type/Map.h>
#include<hgl/type/ObjectList.h>
#include<hgl/log/Log.h>
#include<random>

namespace hgl
{
    namespace audio
    {
        /**
         * 音频源配置
         * 定义单个音频源的特性和生成参数
         */
        struct AudioSourceConfig
        {
            const void* data;           ///< 音频数据指针
            uint dataSize;              ///< 数据大小
            uint format;                ///< 音频格式
            uint sampleRate;            ///< 采样率
            
            // 生成控制参数
            uint minCount;              ///< 最小生成数量
            uint maxCount;              ///< 最大生成数量
            float minInterval;          ///< 最小出现间隔(秒)
            float maxInterval;          ///< 最大出现间隔(秒)
            
            // 变化范围参数
            float minVolume;            ///< 最小音量(0.0-1.0)
            float maxVolume;            ///< 最大音量(0.0-1.0)
            float minPitch;             ///< 最小音调(0.5-2.0)
            float maxPitch;             ///< 最大音调(0.5-2.0)
            
            AudioSourceConfig()
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
            }
        };
        
        /**
         * 音频场景混音器
         * 高级混音控制器，用于管理多种音源的混合
         * 可以控制每种音源的数量、出现频率、变化幅度等
         */
        class AudioScene
        {
            OBJECT_LOGGER
            
        private:
            
            struct SourceEntry
            {
                AudioSourceConfig config;
                OSString name;
            };
            
            Map<OSString, SourceEntry*> sources;    ///< 音频源字典
            MixerConfig globalConfig;               ///< 全局混音配置
            
            std::random_device rd;
            std::mt19937 rng;
            
            /**
             * 生成随机浮点数
             */
            float RandomFloat(float min, float max);
            
            /**
             * 生成随机整数
             */
            uint RandomUInt(uint min, uint max);
            
        public:
            
            AudioScene();
            virtual ~AudioScene();
            
            /**
             * 添加音频源
             * @param name 音频源名称(如"小轿车"、"SUV"、"喇叭"等)
             * @param config 音频源配置
             */
            void AddSource(const OSString& name, const AudioSourceConfig& config);
            
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
             * 生成混音场景
             * @param outputData 输出数据指针(需要调用者释放)
             * @param outputSize 输出数据大小
             * @param outputFormat 输出音频格式
             * @param outputSampleRate 输出采样率
             * @param duration 场景持续时间(秒)
             * @return 是否成功
             */
            bool GenerateScene(void** outputData, uint* outputSize,
                             uint* outputFormat, uint* outputSampleRate,
                             float duration);
            
            /**
             * 生成混音场景(指定输出格式)
             * @param outputData 输出数据指针(需要调用者释放)
             * @param outputSize 输出数据大小
             * @param duration 场景持续时间(秒)
             * @param outputFormat 指定输出格式
             * @param outputSampleRate 指定输出采样率
             * @return 是否成功
             */
            bool GenerateScene(void** outputData, uint* outputSize,
                             float duration,
                             uint outputFormat = AL_FORMAT_MONO16,
                             uint outputSampleRate = 44100);
        };
        
    }//namespace audio
}//namespace hgl
