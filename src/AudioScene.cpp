#include<hgl/audio/AudioScene.h>
#include<hgl/audio/OpenAL.h>
#include<string.h>
#include<algorithm>

using namespace openal;

namespace hgl
{
    namespace audio
    {
        AudioScene::AudioScene() : rng(rd()),
            poolBuffer(OS_TEXT("AudioScene::poolBuffer")),
            tempBuffer(OS_TEXT("AudioScene::tempBuffer"))
        {
            sourceFormat = 0;
            sourceSampleRate = 0;
            outputFormat = AL_FORMAT_MONO16;    // 默认16位单声道
            outputSampleRate = 44100;            // 默认44.1kHz
        }

        AudioScene::~AudioScene()
        {
            ClearSources();
            // 内存池自动释放
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

            if(config.format == 0 || config.sampleRate == 0)
            {
                LogError(OS_TEXT("Invalid format or sample rate for: ") + name);
                return;
            }

            // 如果是第一个音源，设置格式标准
            if(sources.GetCount() == 0)
            {
                sourceFormat = config.format;
                sourceSampleRate = config.sampleRate;
                LogInfo(OS_TEXT("Set source format standard: format=") + OSString::numberOf((int)sourceFormat) +
                       OS_TEXT(", sampleRate=") + OSString::numberOf((int)sourceSampleRate));
            }
            else
            {
                // 验证格式一致性
                if(config.format != sourceFormat || config.sampleRate != sourceSampleRate)
                {
                    LogError(OS_TEXT("Format mismatch for: ") + name +
                            OS_TEXT(". All sources must have format=") + OSString::numberOf((int)sourceFormat) +
                            OS_TEXT(", sampleRate=") + OSString::numberOf((int)sourceSampleRate));
                    return;
                }
            }

            sources.Add(name, config);

            LogInfo(OS_TEXT("Added audio source: ") + name);
        }

        /**
         * 移除音频源
         */
        void AudioScene::RemoveSource(const OSString& name)
        {
            sources.DeleteByKey(name);
        }

        /**
         * 清除所有音频源
         */
        void AudioScene::ClearSources()
        {
            sources.Clear();

            // 重置格式标准
            sourceFormat = 0;
            sourceSampleRate = 0;
        }

        /**
         * 设置输出格式
         */
        void AudioScene::SetOutputFormat(uint format, uint sampleRate)
        {
            outputFormat = format;
            outputSampleRate = sampleRate;

            LogInfo(OS_TEXT("Set output format: format=") + OSString::numberOf((int)outputFormat) +
                   OS_TEXT(", sampleRate=") + OSString::numberOf((int)outputSampleRate));
        }

        /**
         * 生成混音场景 - 使用内存池避免频繁分配
         */
        bool AudioScene::GenerateScene(void** outputData, uint* outputSize, float duration)
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

            LogInfo(OS_TEXT("Generating scene with duration: ") + OSString::floatOf(duration,3) +
                   OS_TEXT(" seconds, output format=") + OSString::numberOf((int)outputFormat) +
                   OS_TEXT(", output sampleRate=") + OSString::numberOf((int)outputSampleRate));

            // 解析音频格式信息 - 仅支持单声道
            AudioDataInfo formatInfo;
            formatInfo.format = outputFormat;
            formatInfo.channels = 1;  // 仅支持单声道
            formatInfo.isFloat = false;

            switch(outputFormat)
            {
                case AL_FORMAT_MONO8:
                    formatInfo.bitsPerSample = 8;
                    break;

                case AL_FORMAT_MONO16:
                    formatInfo.bitsPerSample = 16;
                    break;

                case AL_FORMAT_MONO_FLOAT32:
                    formatInfo.bitsPerSample = 32;
                    formatInfo.isFloat = true;
                    break;

                default:
                    LogError(OS_TEXT("Unsupported audio format (only mono supported)"));
                    RETURN_FALSE;
            }

            formatInfo.sampleRate = outputSampleRate;

            uint bytesPerSample = formatInfo.bitsPerSample / 8;
            uint bytesPerFrame = bytesPerSample;  // 单声道，每帧就是一个采样
            uint totalSamples = (uint)(duration * outputSampleRate);
            uint totalSize = totalSamples * bytesPerFrame;

            // 使用内存池分配输出缓冲区（预分配2倍大小以减少重新分配）
            poolBuffer.EnsureWithEstimate(totalSize, totalSize);

            char* outputBuffer = poolBuffer.Get();
            memset(outputBuffer, 0, totalSize);
            *outputData = outputBuffer;
            *outputSize = totalSize;

            LogInfo(OS_TEXT("Using pool buffer: size=") + OSString::numberOf((int)poolBuffer.GetSize()) +
                   OS_TEXT(" bytes, required=") + OSString::numberOf((int)totalSize) + OS_TEXT(" bytes"));

            // 为每个音源生成实例并混音
            for(auto it:sources)
            {
                const AudioSourceConfig& srcConfig = it->value;

                // 确定生成数量
                uint count = RandomUInt(srcConfig.minCount, srcConfig.maxCount);

                LogInfo(OS_TEXT("Processing source: ") + it->key +
                       OS_TEXT(", generating ") + OSString::numberOf((int)count) +
                       OS_TEXT(" instances"));

                // 为每个实例创建混音器
                float currentTimeOffset = 0.0f;

                for(uint i = 0; i < count; i++)
                {
                    AudioMixer instanceMixer;
                    instanceMixer.SetSourceAudio(srcConfig.data, srcConfig.dataSize,
                                                srcConfig.format, srcConfig.sampleRate);
                    instanceMixer.SetOutputSampleRate(outputSampleRate);  // 设置输出采样率
                    instanceMixer.SetOutputFormat(outputFormat);  // 设置输出格式

                    // 生成随机时间偏移
                    if(i == 0)
                    {
                        // 第一个实例可以在开始时或稍后出现
                        currentTimeOffset = RandomFloat(0.0f, srcConfig.minInterval);
                    }
                    else
                    {
                        // 后续实例基于前一个实例累加间隔
                        float interval = RandomFloat(srcConfig.minInterval, srcConfig.maxInterval);
                        currentTimeOffset += interval;
                    }

                    // 确保不超出持续时间
                    if(currentTimeOffset >= duration)
                        break;

                    // 生成随机音量和音调
                    float volume = RandomFloat(srcConfig.minVolume, srcConfig.maxVolume);
                    float pitch = RandomFloat(srcConfig.minPitch, srcConfig.maxPitch);

                    // 添加单个轨道
                    instanceMixer.AddTrack(currentTimeOffset, volume, pitch);

                    // 混音到临时缓冲区(使用temp buffer避免频繁分配)
                    void* instanceData = nullptr;
                    uint instanceSize = 0;

                    if(instanceMixer.Mix(&instanceData, &instanceSize, duration))
                    {
                        // 确保临时缓冲区足够大以容纳实例数据
                        tempBuffer.Ensure(instanceSize);

                        // 复制到临时缓冲区(这样可以让AudioMixer内部复用它的池)
                        memcpy(tempBuffer.Get(), instanceData, instanceSize);

                        // 将实例数据混合到输出
                        if(formatInfo.isFloat && formatInfo.bitsPerSample == 32)
                        {
                            // Float混音 - 简单直接相加，无需担心溢出
                            float* outputSamples = (float*)outputBuffer;
                            const float* instanceSamples = (const float*)tempBuffer.Get();
                            uint outputSampleCount = totalSize / sizeof(float);
                            uint instanceSampleCount = instanceSize / sizeof(float);
                            uint sampleCount = std::min(instanceSampleCount, outputSampleCount);

                            for(uint s = 0; s < sampleCount; s++)
                            {
                                outputSamples[s] += instanceSamples[s];
                            }
                        }
                        else if(formatInfo.bitsPerSample == 16)
                        {
                            int16_t* outputSamples = (int16_t*)outputBuffer;
                            const int16_t* instanceSamples = (const int16_t*)tempBuffer.Get();
                            uint outputSampleCount = totalSize / sizeof(int16_t);
                            uint instanceSampleCount = instanceSize / sizeof(int16_t);
                            uint sampleCount = std::min(instanceSampleCount, outputSampleCount);

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
                            int8_t* outputSamples = (int8_t*)outputBuffer;
                            const int8_t* instanceSamples = (const int8_t*)tempBuffer.Get();
                            uint outputSampleCount = totalSize / sizeof(int8_t);
                            uint instanceSampleCount = instanceSize / sizeof(int8_t);
                            uint sampleCount = std::min(instanceSampleCount, outputSampleCount);

                            for(uint s = 0; s < sampleCount; s++)
                            {
                                int16_t mixed = outputSamples[s] + instanceSamples[s];

                                // 限幅
                                if(mixed > 127) mixed = 127;
                                if(mixed < -128) mixed = -128;

                                outputSamples[s] = (int8_t)mixed;
                            }
                        }
                    }
                }
            }

            LogInfo(OS_TEXT("Scene generation completed successfully"));
            return(true);
        }
    }//namespace audio
}//namespace hgl
