#include<hgl/audio/AudioMixerScene.h>
#include<hgl/audio/OpenAL.h>
#include<string.h>
#include<algorithm>
#include<vector>

using namespace openal;

namespace hgl
{
    namespace audio
    {
        void ApplyAudioFilterPreset(AudioMixerSourceConfig &config, AudioFilterPreset preset)
        {
            config.filterConfig = GetAudioFilterPresetConfig(preset);
        }

        AudioMixerScene::AudioMixerScene() : rng(rd()),
            poolBuffer(OS_TEXT("AudioMixerScene::poolBuffer")),
            tempBuffer(OS_TEXT("AudioMixerScene::tempBuffer"))
        {
            sourceFormat = 0;
            sourceSampleRate = 0;
            outputFormat = AL_FORMAT_MONO16;    // 默认16位单声道
            outputSampleRate = 44100;            // 默认44.1kHz
        }

        AudioMixerScene::~AudioMixerScene()
        {
            ClearSources();
            // 内存池自动释放
        }

        /**
         * 生成随机浮点数
         */
        float AudioMixerScene::RandomFloat(float min, float max)
        {
            std::uniform_real_distribution<float> dist(min, max);
            return dist(rng);
        }

        /**
         * 生成随机整数
         */
        uint AudioMixerScene::RandomUInt(uint min, uint max)
        {
            std::uniform_int_distribution<uint> dist(min, max);
            return dist(rng);
        }

        bool AudioMixerScene::HasAnyEffects() const
        {
            for(auto& [sourceName, srcConfig] : sources)
            {
                if((srcConfig.filterConfig.enable && srcConfig.filterConfig.type != AudioFilterType::None) || srcConfig.reverb.enable)
                    return true;
            }

            return false;
        }

        AudioFilterConfig AudioMixerScene::BuildRandomFilterConfig(const AudioMixerSourceConfig& config)
        {
            AudioFilterConfig filter = config.filterConfig;
            if(!filter.enable || filter.type == AudioFilterType::None)
                return filter;

            auto jitter = [this](float base, float range) -> float
            {
                if(range <= 0.0f)
                    return std::clamp(base, 0.0f, 1.0f);

                return std::clamp(base + RandomFloat(-range, range), 0.0f, 1.0f);
            };

            filter.gain = jitter(filter.gain, config.filterRandom.gain);
            filter.gain_lf = jitter(filter.gain_lf, config.filterRandom.gain_lf);
            filter.gain_hf = jitter(filter.gain_hf, config.filterRandom.gain_hf);

            return filter;
        }

        void AudioMixerScene::ApplyLowpass(float* samples, uint count, float alpha)
        {
            if(!samples || count == 0)
                return;

            float y = samples[0];
            for(uint i = 0; i < count; i++)
            {
                y = y + alpha * (samples[i] - y);
                samples[i] = y;
            }
        }

        void AudioMixerScene::ApplyHighpass(float* samples, uint count, float alpha)
        {
            if(!samples || count == 0)
                return;

            float y = 0.0f;
            float x_prev = samples[0];

            for(uint i = 0; i < count; i++)
            {
                float x = samples[i];
                y = alpha * (y + x - x_prev);
                samples[i] = y;
                x_prev = x;
            }
        }

        void AudioMixerScene::ApplyFilter(float* samples, uint count, const AudioFilterConfig& config)
        {
            if(!config.enable || config.type == AudioFilterType::None)
                return;

            float alpha_lp = std::clamp(config.gain_hf, 0.01f, 1.0f);
            float alpha_hp = std::clamp(1.0f - config.gain_lf, 0.01f, 1.0f);

            switch(config.type)
            {
                case AudioFilterType::Lowpass:
                    ApplyLowpass(samples, count, alpha_lp);
                    break;
                case AudioFilterType::Highpass:
                    ApplyHighpass(samples, count, alpha_hp);
                    break;
                case AudioFilterType::Bandpass:
                    ApplyLowpass(samples, count, alpha_lp);
                    ApplyHighpass(samples, count, alpha_hp);
                    break;
                default:
                    break;
            }

            if(config.gain != 1.0f)
            {
                for(uint i = 0; i < count; i++)
                {
                    samples[i] *= config.gain;
                }
            }
        }

        void AudioMixerScene::ApplySimpleReverb(float* samples, uint count, uint sampleRate, const AudioMixerSourceConfig::SimpleReverbConfig& config)
        {
            if(!config.enable || count == 0 || sampleRate == 0)
                return;

            float delay_ms = std::clamp(config.delay_ms, 1.0f, 200.0f);
            float feedback = std::clamp(config.feedback, 0.0f, 0.95f);
            float mix = std::clamp(config.mix, 0.0f, 1.0f);

            uint delay_samples = (uint)(delay_ms * (float)sampleRate / 1000.0f);
            if(delay_samples < 1)
                return;

            std::vector<float> delayBuffer(delay_samples, 0.0f);
            uint index = 0;

            for(uint i = 0; i < count; i++)
            {
                float dry = samples[i];
                float delayed = delayBuffer[index];
                float wet = delayed;

                samples[i] = dry * (1.0f - mix) + wet * mix;
                delayBuffer[index] = dry + delayed * feedback;

                index++;
                if(index >= delay_samples)
                    index = 0;
            }
        }


        bool AudioMixerScene::ConvertFloatToOutput(const float* input, uint sampleCount, void** outputData, uint* outputSize)
        {
            if(!input || !outputData || !outputSize)
                return(false);

            if(outputFormat == AL_FORMAT_MONO_FLOAT32)
            {
                *outputData = (void*)input;
                *outputSize = sampleCount * sizeof(float);
                return(true);
            }

            if(outputFormat == AL_FORMAT_MONO16)
            {
                uint totalSize = sampleCount * sizeof(int16_t);
                tempBuffer.Ensure(totalSize);
                int16_t* output = (int16_t*)tempBuffer.Get();

                for(uint i = 0; i < sampleCount; i++)
                {
                    float sample = std::clamp(input[i], -1.0f, 1.0f);
                    output[i] = (int16_t)(sample * 32767.0f);
                }

                *outputData = output;
                *outputSize = totalSize;
                return(true);
            }

            if(outputFormat == AL_FORMAT_MONO8)
            {
                uint totalSize = sampleCount * sizeof(int8_t);
                tempBuffer.Ensure(totalSize);
                int8_t* output = (int8_t*)tempBuffer.Get();

                for(uint i = 0; i < sampleCount; i++)
                {
                    float sample = std::clamp(input[i], -1.0f, 1.0f);
                    output[i] = (int8_t)(sample * 127.0f);
                }

                *outputData = output;
                *outputSize = totalSize;
                return(true);
            }

            return(false);
        }

        /**
         * 添加音频源
         */
        void AudioMixerScene::AddSource(const OSString& name, const AudioMixerSourceConfig& config)
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
        void AudioMixerScene::RemoveSource(const OSString& name)
        {
            sources.DeleteByKey(name);
        }

        /**
         * 清除所有音频源
         */
        void AudioMixerScene::ClearSources()
        {
            sources.Clear();

            // 重置格式标准
            sourceFormat = 0;
            sourceSampleRate = 0;
        }

        /**
         * 设置输出格式
         */
        void AudioMixerScene::SetOutputFormat(uint format, uint sampleRate)
        {
            outputFormat = format;
            outputSampleRate = sampleRate;

            LogInfo(OS_TEXT("Set output format: format=") + OSString::numberOf((int)outputFormat) +
                   OS_TEXT(", sampleRate=") + OSString::numberOf((int)outputSampleRate));
        }

        /**
         * 生成混音场景 - 使用内存池避免频繁分配
         */
        bool AudioMixerScene::GenerateScene(void** outputData, uint* outputSize, float duration)
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

            if(outputFormat != AL_FORMAT_MONO8 &&
               outputFormat != AL_FORMAT_MONO16 &&
               outputFormat != AL_FORMAT_MONO_FLOAT32)
            {
                LogError(OS_TEXT("Unsupported audio format (only mono supported)"));
                RETURN_FALSE;
            }

            if(outputSampleRate != sourceSampleRate)
            {
                LogError(OS_TEXT("Output sample rate must match source sample rate for AudioMixer"));
                RETURN_FALSE;
            }

            bool mix_in_float = (outputFormat == AL_FORMAT_MONO_FLOAT32) || HasAnyEffects();

            uint bytesPerSample = mix_in_float ? sizeof(float) : (outputFormat == AL_FORMAT_MONO8 ? 1 : 2);
            uint bytesPerFrame = bytesPerSample;  // 单声道，每帧就是一个采样
            uint totalSamples = (uint)(duration * outputSampleRate);
            uint totalSize = totalSamples * bytesPerFrame;

            // 使用内存池分配输出缓冲区（预分配2倍大小以减少重新分配）
            poolBuffer.EnsureWithEstimate(totalSize, totalSize);

            char* outputBuffer = poolBuffer.Get();
            memset(outputBuffer, 0, totalSize);

            LogInfo(OS_TEXT("Using pool buffer: size=") + OSString::numberOf((int)poolBuffer.GetSize()) +
                   OS_TEXT(" bytes, required=") + OSString::numberOf((int)totalSize) + OS_TEXT(" bytes"));

            // 为每个音源生成实例并混音
            for(auto& [sourceName, srcConfig] : sources)
            {
                // 确定生成数量
                uint count = RandomUInt(srcConfig.minCount, srcConfig.maxCount);

                LogInfo(OS_TEXT("Processing source: ") + sourceName +
                       OS_TEXT(", generating ") + OSString::numberOf((int)count) +
                       OS_TEXT(" instances"));

                // 为每个实例创建混音器
                float currentTimeOffset = 0.0f;

                for(uint i = 0; i < count; i++)
                {
                    AudioMixer instanceMixer;
                    int sourceIndex = instanceMixer.AddSourceAudio(srcConfig.data, srcConfig.dataSize,
                                                                  srcConfig.format, srcConfig.sampleRate);
                    if(sourceIndex < 0)
                    {
                        LogError(OS_TEXT("Failed to add source audio for mixer instance"));
                        RETURN_FALSE;
                    }
                    instanceMixer.SetOutputFormat(mix_in_float ? AL_FORMAT_MONO_FLOAT32 : outputFormat);  // 设置输出格式

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
                    instanceMixer.AddTrack((uint)sourceIndex, currentTimeOffset, volume, pitch);

                    // 混音到临时缓冲区(使用temp buffer避免频繁分配)
                    void* instanceData = nullptr;
                    uint instanceSize = 0;

                    if(instanceMixer.Mix(&instanceData, &instanceSize, duration))
                    {
                        // 确保临时缓冲区足够大以容纳实例数据
                        tempBuffer.Ensure(instanceSize);

                        // 复制到临时缓冲区(这样可以让AudioMixer内部复用它的池)
                        memcpy(tempBuffer.Get(), instanceData, instanceSize);

                        if(mix_in_float)
                        {
                            float* instanceSamples = (float*)tempBuffer.Get();
                            uint instanceSampleCount = instanceSize / sizeof(float);
                            uint outputSampleCount = totalSize / sizeof(float);
                            uint sampleCount = std::min(instanceSampleCount, outputSampleCount);

                            if((srcConfig.filterConfig.enable && srcConfig.filterConfig.type != AudioFilterType::None) || srcConfig.reverb.enable)
                            {
                                AudioFilterConfig filter = BuildRandomFilterConfig(srcConfig);

                                AudioMixerSourceConfig::SimpleReverbConfig reverb = srcConfig.reverb;
                                if(reverb.enable)
                                {
                                    if(reverb.delay_ms_rand != 0.0f)
                                        reverb.delay_ms = std::clamp(reverb.delay_ms + RandomFloat(-reverb.delay_ms_rand, reverb.delay_ms_rand), 1.0f, 200.0f);
                                    if(reverb.feedback_rand != 0.0f)
                                        reverb.feedback = std::clamp(reverb.feedback + RandomFloat(-reverb.feedback_rand, reverb.feedback_rand), 0.0f, 0.95f);
                                    if(reverb.mix_rand != 0.0f)
                                        reverb.mix = std::clamp(reverb.mix + RandomFloat(-reverb.mix_rand, reverb.mix_rand), 0.0f, 1.0f);
                                }

                                ApplyFilter(instanceSamples, sampleCount, filter);
                                ApplySimpleReverb(instanceSamples, sampleCount, outputSampleRate, reverb);
                            }

                            float* outputSamples = (float*)outputBuffer;
                            for(uint s = 0; s < sampleCount; s++)
                            {
                                outputSamples[s] += instanceSamples[s];
                            }
                        }
                        else if(outputFormat == AL_FORMAT_MONO16)
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
                        else if(outputFormat == AL_FORMAT_MONO8)
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

            if(mix_in_float)
            {
                float* mixSamples = (float*)outputBuffer;
                if(!ConvertFloatToOutput(mixSamples, totalSamples, outputData, outputSize))
                    RETURN_FALSE;
            }
            else
            {
                *outputData = outputBuffer;
                *outputSize = totalSize;
            }

            LogInfo(OS_TEXT("Scene generation completed successfully"));
            return(true);
        }
    }//namespace audio
}//namespace hgl
