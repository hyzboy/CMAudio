#include<hgl/audio/AudioScene.h>
#include<hgl/audio/OpenAL.h>
#include<string.h>
#include<algorithm>

using namespace openal;

namespace hgl
{
    namespace audio
    {
        AudioScene::AudioScene() : rng(rd())
        {
        }
        
        AudioScene::~AudioScene()
        {
            ClearSources();
        }
        
        /**
         * 生成随机浮点数
         */
        float AudioScene::RandomFloat(float min, float max)
        {
            std::uniform_real_distribution<float> dist(min, max);
            return dist(rng);
        }
        
        /**
         * 生成随机整数
         */
        uint AudioScene::RandomUInt(uint min, uint max)
        {
            std::uniform_int_distribution<uint> dist(min, max);
            return dist(rng);
        }
        
        /**
         * 添加音频源
         */
        void AudioScene::AddSource(const OSString& name, const AudioSourceConfig& config)
        {
            if(!config.data || config.dataSize == 0)
            {
                LogError(OS_TEXT("Invalid audio source data for: ") + name);
                return;
            }
            
            SourceEntry* entry = new SourceEntry();
            entry->config = config;
            entry->name = name;
            
            sources.Add(name, entry);
            
            LogInfo(OS_TEXT("Added audio source: ") + name);
        }
        
        /**
         * 移除音频源
         */
        void AudioScene::RemoveSource(const OSString& name)
        {
            SourceEntry* entry = nullptr;
            if(sources.Get(name, entry))
            {
                sources.Delete(name);
                delete entry;
            }
        }
        
        /**
         * 清除所有音频源
         */
        void AudioScene::ClearSources()
        {
            auto it = sources.GetIterator();
            SourceEntry* entry;
            
            while(it.Next())
            {
                entry = it.GetValue();
                delete entry;
            }
            
            sources.Clear();
        }
        
        /**
         * 生成混音场景
         */
        bool AudioScene::GenerateScene(void** outputData, uint* outputSize,
                                      uint* outputFormat, uint* outputSampleRate,
                                      float duration)
        {
            if(sources.GetCount() == 0)
            {
                LogError(OS_TEXT("No audio sources configured"));
                RETURN_FALSE;
            }
            
            if(duration <= 0.0f)
            {
                LogError(OS_TEXT("Invalid duration"));
                RETURN_FALSE;
            }
            
            // 确定输出格式(使用第一个源的格式)
            auto it = sources.GetIterator();
            if(!it.Next())
            {
                RETURN_FALSE;
            }
            
            SourceEntry* firstEntry = it.GetValue();
            uint targetFormat = firstEntry->config.format;
            uint targetSampleRate = firstEntry->config.sampleRate;
            
            LogInfo(OS_TEXT("Generating scene with duration: ") + OSString::floatOf(duration) + OS_TEXT(" seconds"));
            
            // 创建主混音器来合并所有源
            AudioMixer mainMixer;
            mainMixer.SetConfig(globalConfig);
            
            // 解析音频格式信息
            AudioDataInfo formatInfo;
            formatInfo.format = targetFormat;
            
            switch(targetFormat)
            {
                case AL_FORMAT_MONO8:
                    formatInfo.channels = 1;
                    formatInfo.bitsPerSample = 8;
                    break;
                    
                case AL_FORMAT_MONO16:
                    formatInfo.channels = 1;
                    formatInfo.bitsPerSample = 16;
                    break;
                    
                case AL_FORMAT_STEREO8:
                    formatInfo.channels = 2;
                    formatInfo.bitsPerSample = 8;
                    break;
                    
                case AL_FORMAT_STEREO16:
                    formatInfo.channels = 2;
                    formatInfo.bitsPerSample = 16;
                    break;
                    
                default:
                    LogError(OS_TEXT("Unsupported audio format"));
                    RETURN_FALSE;
            }
            
            formatInfo.sampleRate = targetSampleRate;
            
            uint bytesPerSample = formatInfo.bitsPerSample / 8;
            uint bytesPerFrame = bytesPerSample * formatInfo.channels;
            uint totalSamples = (uint)(duration * targetSampleRate);
            uint totalSize = totalSamples * bytesPerFrame;
            
            // 分配并初始化输出缓冲区
            *outputData = new char[totalSize];
            memset(*outputData, 0, totalSize);
            *outputSize = totalSize;
            *outputFormat = targetFormat;
            *outputSampleRate = targetSampleRate;
            
            // 为每个音源生成实例并混音
            it = sources.GetIterator();
            while(it.Next())
            {
                SourceEntry* entry = it.GetValue();
                const AudioSourceConfig& srcConfig = entry->config;
                
                // 确定生成数量
                uint count = RandomUInt(srcConfig.minCount, srcConfig.maxCount);
                
                LogInfo(OS_TEXT("Processing source: ") + entry->name + 
                       OS_TEXT(", generating ") + OSString::numberOf((int)count) + 
                       OS_TEXT(" instances"));
                
                // 为每个实例创建混音器
                for(uint i = 0; i < count; i++)
                {
                    AudioMixer instanceMixer;
                    instanceMixer.SetSourceAudio(srcConfig.data, srcConfig.dataSize, 
                                                srcConfig.format, srcConfig.sampleRate);
                    
                    // 生成随机时间偏移
                    float timeOffset;
                    if(i == 0)
                    {
                        timeOffset = RandomFloat(0.0f, srcConfig.minInterval);
                    }
                    else
                    {
                        // 基于前一个实例的间隔
                        float interval = RandomFloat(srcConfig.minInterval, srcConfig.maxInterval);
                        timeOffset = interval * i;
                        
                        // 确保不超出持续时间
                        if(timeOffset >= duration)
                            break;
                    }
                    
                    // 生成随机音量和音调
                    float volume = RandomFloat(srcConfig.minVolume, srcConfig.maxVolume);
                    float pitch = RandomFloat(srcConfig.minPitch, srcConfig.maxPitch);
                    
                    // 添加单个轨道
                    instanceMixer.AddTrack(timeOffset, volume, pitch);
                    
                    // 混音到临时缓冲区
                    void* instanceData = nullptr;
                    uint instanceSize = 0;
                    
                    if(instanceMixer.Mix(&instanceData, &instanceSize, duration))
                    {
                        // 将实例数据混合到输出
                        if(formatInfo.bitsPerSample == 16)
                        {
                            int16_t* outputSamples = (int16_t*)*outputData;
                            const int16_t* instanceSamples = (const int16_t*)instanceData;
                            uint sampleCount = std::min(instanceSize, totalSize) / 2;
                            
                            for(uint s = 0; s < sampleCount; s++)
                            {
                                int32_t mixed = outputSamples[s] + instanceSamples[s];
                                
                                // 限幅
                                if(mixed > 32767) mixed = 32767;
                                if(mixed < -32768) mixed = -32768;
                                
                                outputSamples[s] = (int16_t)mixed;
                            }
                        }
                        else if(formatInfo.bitsPerSample == 8)
                        {
                            int8_t* outputSamples = (int8_t*)*outputData;
                            const int8_t* instanceSamples = (const int8_t*)instanceData;
                            uint sampleCount = std::min(instanceSize, totalSize);
                            
                            for(uint s = 0; s < sampleCount; s++)
                            {
                                int16_t mixed = outputSamples[s] + instanceSamples[s];
                                
                                // 限幅
                                if(mixed > 127) mixed = 127;
                                if(mixed < -128) mixed = -128;
                                
                                outputSamples[s] = (int8_t)mixed;
                            }
                        }
                        
                        delete[] (char*)instanceData;
                    }
                }
            }
            
            LogInfo(OS_TEXT("Scene generation completed successfully"));
            RETURN_TRUE;
        }
        
        /**
         * 生成混音场景(指定输出格式)
         */
        bool AudioScene::GenerateScene(void** outputData, uint* outputSize,
                                      float duration,
                                      uint outputFormat,
                                      uint outputSampleRate)
        {
            uint format = outputFormat;
            uint sampleRate = outputSampleRate;
            
            return GenerateScene(outputData, outputSize, &format, &sampleRate, duration);
        }
        
    }//namespace audio
}//namespace hgl
