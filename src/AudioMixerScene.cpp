#include<hgl/audio/AudioMixerScene.h>
#include<string.h>
#include<algorithm>
#include<vector>
#include<cstdint>

using namespace openal;

namespace hgl::audio
{
        void ApplyAudioFilterPreset(AudioMixerSourceConfig &config, AudioFilterPreset preset)
        {
            config.filter_config = GetAudioFilterPresetConfig(preset);
        }

        AudioMixerScene::AudioMixerScene() : rng(random_device()),
            pool_buffer(OS_TEXT("AudioMixerScene::pool_buffer")),
            temp_buffer(OS_TEXT("AudioMixerScene::temp_buffer"))
        {
            output_format.channels = 1;      // 默认16位单声道
            output_format.bits_per_sample = 16;
            output_format.is_float = false;
            output_format.sample_rate = 44100; // 默认44.1kHz
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
                if((srcConfig.filter_config.enable && srcConfig.filter_config.filter_type != AudioFilterType::None) || srcConfig.reverb.enable)
                    return true;
            }

            return false;
        }

        AudioFilterConfig AudioMixerScene::BuildRandomFilterConfig(const AudioMixerSourceConfig& config)
        {
            AudioFilterConfig filter = config.filter_config;
            if(!filter.enable || filter.filter_type == AudioFilterType::None)
                return filter;

            auto jitter = [this](float base, float range) -> float
            {
                if(range <= 0.0f)
                    return std::clamp(base, 0.0f, 1.0f);

                return std::clamp(base + RandomFloat(-range, range), 0.0f, 1.0f);
            };

            filter.gain = jitter(filter.gain, config.filter_random.gain);
            filter.gain_lf = jitter(filter.gain_lf, config.filter_random.gain_lf);
            filter.gain_hf = jitter(filter.gain_hf, config.filter_random.gain_hf);

            return filter;
        }

        void AudioMixerScene::ApplyLowpass(float* samples, uint count, uint channels, float alpha)
        {
            if(!samples || count == 0)
                return;

            if(channels == 0)
                channels = 1;

            for(uint ch = 0; ch < channels; ch++)
            {
                float y = samples[ch];
                for(uint i = ch; i < count; i += channels)
                {
                    y = y + alpha * (samples[i] - y);
                    samples[i] = y;
                }
            }
        }

        void AudioMixerScene::ApplyHighpass(float* samples, uint count, uint channels, float alpha)
        {
            if(!samples || count == 0)
                return;

            if(channels == 0)
                channels = 1;

            for(uint ch = 0; ch < channels; ch++)
            {
                float y = 0.0f;
                float x_prev = samples[ch];

                for(uint i = ch; i < count; i += channels)
                {
                    float x = samples[i];
                    y = alpha * (y + x - x_prev);
                    samples[i] = y;
                    x_prev = x;
                }
            }
        }

        void AudioMixerScene::ApplyFilter(float* samples, uint count, uint channels, const AudioFilterConfig& config)
        {
            if(!config.enable || config.filter_type == AudioFilterType::None)
                return;

            float alpha_lp = std::clamp(config.gain_hf, 0.01f, 1.0f);
            float alpha_hp = std::clamp(1.0f - config.gain_lf, 0.01f, 1.0f);

            switch(config.filter_type)
            {
                case AudioFilterType::Lowpass:
                    ApplyLowpass(samples, count, channels, alpha_lp);
                    break;
                case AudioFilterType::Highpass:
                    ApplyHighpass(samples, count, channels, alpha_hp);
                    break;
                case AudioFilterType::Bandpass:
                    ApplyLowpass(samples, count, channels, alpha_lp);
                    ApplyHighpass(samples, count, channels, alpha_hp);
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

        void AudioMixerScene::ApplySimpleReverb(float* samples, uint count, uint channels, uint sample_rate, const AudioMixerSourceConfig::SimpleReverbConfig& config)
        {
            if(!config.enable || count == 0 || sample_rate == 0)
                return;

            if(channels == 0)
                channels = 1;

            float delay_ms = std::clamp(config.delay_ms, 1.0f, 200.0f);
            float feedback = std::clamp(config.feedback, 0.0f, 0.95f);
            float mix = std::clamp(config.mix, 0.0f, 1.0f);

            uint delay_samples = (uint)(delay_ms * (float)sample_rate / 1000.0f);
            if(delay_samples < 1)
                return;

            // 每个声道独立使用一个延迟缓冲
            std::vector<float> delayBuffer(delay_samples, 0.0f);

            for(uint ch = 0; ch < channels; ch++)
            {
                std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
                uint index = 0;

                for(uint i = ch; i < count; i += channels)
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
        }


        bool AudioMixerScene::ConvertFloatToOutput(const float* input, uint sampleCount, void** outputData, uint* outputSize)
        {
            if(!input || !outputData || !outputSize)
                return(false);

            const AudioDataInfo &outInfo = output_format;

            if(outInfo.is_float && outInfo.bits_per_sample == 32)
            {
                *outputData = (void*)input;
                *outputSize = sampleCount * sizeof(float);
                return(true);
            }

            if(outInfo.bits_per_sample == 32)
            {
                uint totalSize = sampleCount * sizeof(int32_t);
                temp_buffer.Ensure(totalSize);
                int32_t* output = (int32_t*)temp_buffer.Get();

                for(uint i = 0; i < sampleCount; i++)
                {
                    float sample = std::clamp(input[i], -1.0f, 1.0f);
                    output[i] = (int32_t)((double)sample * 2147483647.0);
                }

                *outputData = output;
                *outputSize = totalSize;
                return(true);
            }

            if(outInfo.bits_per_sample == 16)
            {
                uint totalSize = sampleCount * sizeof(int16_t);
                temp_buffer.Ensure(totalSize);
                int16_t* output = (int16_t*)temp_buffer.Get();

                for(uint i = 0; i < sampleCount; i++)
                {
                    float sample = std::clamp(input[i], -1.0f, 1.0f);
                    output[i] = (int16_t)(sample * 32767.0f);
                }

                *outputData = output;
                *outputSize = totalSize;
                return(true);
            }

            if(outInfo.bits_per_sample == 8)
            {
                uint totalSize = sampleCount * sizeof(int8_t);
                temp_buffer.Ensure(totalSize);
                int8_t* output = (int8_t*)temp_buffer.Get();

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
            if(!config.data || config.info.data_size == 0)
            {
                LogError(OS_TEXT("Invalid audio source data for: ") + name);
                return;
            }

            if(config.info.sample_rate == 0 || config.info.channels == 0 || config.info.bits_per_sample == 0)
            {
                LogError(OS_TEXT("Invalid format or sample rate for: ") + name);
                return;
            }

            // 如果是第一个音源，设置格式标准
            if(sources.GetCount() == 0)
            {
                source_format = config.info;
                LogInfo(OS_TEXT("Set source format standard: sample_rate=") + OSString::numberOf((int)source_format.sample_rate) +
                       OS_TEXT(", channels=") + OSString::numberOf((int)source_format.channels));
            }
            else
            {
                // 验证格式一致性
                if(config.info.sample_rate != source_format.sample_rate ||
                   config.info.channels != source_format.channels ||
                   config.info.bits_per_sample != source_format.bits_per_sample ||
                   config.info.is_float != source_format.is_float)
                {
                    LogError(OS_TEXT("Format mismatch for: ") + name +
                            OS_TEXT(". All sources must have sample_rate=") + OSString::numberOf((int)source_format.sample_rate) +
                            OS_TEXT(", channels=") + OSString::numberOf((int)source_format.channels));
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
            source_format = AudioDataInfo();
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
                   OS_TEXT(" seconds, output channels=") + OSString::numberOf((int)output_format.channels) +
                   OS_TEXT(", output sample_rate=") + OSString::numberOf((int)output_format.sample_rate));

            const AudioDataInfo &sourceInfo = source_format;

            const uint channels = sourceInfo.channels ? sourceInfo.channels : 1;

            const AudioDataInfo &outInfo = output_format;

            if(outInfo.channels != channels)
            {
                LogError(OS_TEXT("Output channel count must match source channel count"));
                RETURN_FALSE;
            }

            if(output_format.sample_rate != sourceInfo.sample_rate)
            {
                LogError(OS_TEXT("Output sample rate must match source sample rate for AudioMixer"));
                RETURN_FALSE;
            }

            bool mix_in_float = outInfo.is_float || HasAnyEffects();

            uint bytesPerSample = mix_in_float ? sizeof(float) : (outInfo.bits_per_sample / 8);
            uint bytesPerFrame = bytesPerSample * channels;
            uint totalFrames = (uint)(duration * output_format.sample_rate);
            uint totalSamples = totalFrames * channels;
            uint totalSize = totalFrames * bytesPerFrame;

            // 使用内存池分配输出缓冲区
            pool_buffer.Ensure(totalSize);

            char* outputBuffer = pool_buffer.Get();
            memset(outputBuffer, 0, totalSize);

            LogInfo(OS_TEXT("Using pool buffer: size=") + OSString::numberOf((int)pool_buffer.GetSize()) +
                   OS_TEXT(" bytes, required=") + OSString::numberOf((int)totalSize) + OS_TEXT(" bytes"));

            // 为每个音源生成实例并混音
            for(auto& [sourceName, srcConfig] : sources)
            {
                // 确定生成数量
                uint count = RandomUInt(srcConfig.min_count, srcConfig.max_count);

                LogInfo(OS_TEXT("Processing source: ") + sourceName +
                       OS_TEXT(", generating ") + OSString::numberOf((int)count) +
                       OS_TEXT(" instances"));

                // 为每个实例创建混音器
                float currentTimeOffset = 0.0f;

                for(uint i = 0; i < count; i++)
                {
                    AudioMixer instanceMixer;
                    int source_index = instanceMixer.AddSourceAudio(srcConfig.info, srcConfig.data);
                    if(source_index < 0)
                    {
                        LogError(OS_TEXT("Failed to add source audio for mixer instance"));
                        RETURN_FALSE;
                    }
                    // 浮点混音时，AudioMixer::Mix 会直接透传按源声道数交织的 float 缓冲，
                    // 输出格式声明的声道数被忽略，因此 float32 仅作为占位符。
                    AudioDataInfo instanceOutputFormat = output_format;
                    if(mix_in_float)
                    {
                        instanceOutputFormat.channels = srcConfig.info.channels;
                        instanceOutputFormat.bits_per_sample = 32;
                        instanceOutputFormat.is_float = true;
                    }
                    instanceMixer.SetOutputFormat(instanceOutputFormat);

                    // 生成随机时间偏移
                    if(i == 0)
                    {
                        // 第一个实例可以在开始时或稍后出现
                        currentTimeOffset = RandomFloat(0.0f, srcConfig.min_interval);
                    }
                    else
                    {
                        // 后续实例基于前一个实例累加间隔
                        float interval = RandomFloat(srcConfig.min_interval, srcConfig.max_interval);
                        currentTimeOffset += interval;
                    }

                    // 确保不超出持续时间
                    if(currentTimeOffset >= duration)
                        break;

                    // 生成随机音量和音调
                    float volume = RandomFloat(srcConfig.min_volume, srcConfig.max_volume);
                    float pitch = RandomFloat(srcConfig.min_pitch, srcConfig.max_pitch);

                    // 添加单个轨道
                    instanceMixer.AddTrack((uint)source_index, currentTimeOffset, volume, pitch);

                    // 混音到临时缓冲区(使用temp buffer避免频繁分配)
                    void* instanceData = nullptr;
                    uint instanceSize = 0;

                    if(instanceMixer.Mix(&instanceData, &instanceSize, duration))
                    {
                        // 确保临时缓冲区足够大以容纳实例数据
                        temp_buffer.Ensure(instanceSize);

                        // 复制到临时缓冲区(这样可以让AudioMixer内部复用它的池)
                        memcpy(temp_buffer.Get(), instanceData, instanceSize);

                        if(mix_in_float)
                        {
                            float* instanceSamples = (float*)temp_buffer.Get();
                            uint instanceSampleCount = instanceSize / sizeof(float);
                            uint outputSampleCount = totalSize / sizeof(float);
                            uint sampleCount = std::min(instanceSampleCount, outputSampleCount);

                            if((srcConfig.filter_config.enable && srcConfig.filter_config.filter_type != AudioFilterType::None) || srcConfig.reverb.enable)
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

                                ApplyFilter(instanceSamples, sampleCount, channels, filter);
                                ApplySimpleReverb(instanceSamples, sampleCount, channels, output_format.sample_rate, reverb);
                            }

                            float* outputSamples = (float*)outputBuffer;
                            for(uint s = 0; s < sampleCount; s++)
                            {
                                outputSamples[s] += instanceSamples[s];
                            }
                        }
                        else if(outInfo.bits_per_sample == 16)
                        {
                            int16_t* outputSamples = (int16_t*)outputBuffer;
                            const int16_t* instanceSamples = (const int16_t*)temp_buffer.Get();
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
                        else if(outInfo.bits_per_sample == 8)
                        {
                            int8_t* outputSamples = (int8_t*)outputBuffer;
                            const int8_t* instanceSamples = (const int8_t*)temp_buffer.Get();
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
                        else if(outInfo.bits_per_sample == 32)
                        {
                            int32_t* outputSamples = (int32_t*)outputBuffer;
                            const int32_t* instanceSamples = (const int32_t*)temp_buffer.Get();
                            uint outputSampleCount = totalSize / sizeof(int32_t);
                            uint instanceSampleCount = instanceSize / sizeof(int32_t);
                            uint sampleCount = std::min(instanceSampleCount, outputSampleCount);

                            for(uint s = 0; s < sampleCount; s++)
                            {
                                int64_t mixed = (int64_t)outputSamples[s] + instanceSamples[s];

                                // 限幅
                                if(mixed > 2147483647LL) mixed = 2147483647LL;
                                if(mixed < -2147483648LL) mixed = -2147483648LL;

                                outputSamples[s] = (int32_t)mixed;
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
}//namespace hgl::audio
