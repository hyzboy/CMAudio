#include<hgl/audio/AudioMixer.h>
#include<hgl/audio/OpenAL.h>
#include<hgl/type/Smart.h>
#include<math.h>
#include<string.h>
#include<cstdint>

using namespace openal;

namespace hgl::audio
{
        AudioMixer::AudioMixer()
            : pool_buffer(OS_TEXT("AudioMixer::pool_buffer")),
              temp_buffer(OS_TEXT("AudioMixer::temp_buffer"))
        {
            has_common_info = false;
            output_format.channels = 1;      // 默认单声道
            output_format.bits_per_sample = 16; // 默认输出int16
            output_format.is_float = false;
        }

        AudioMixer::~AudioMixer()
        {
            ClearTracks();
            // 内存池自动释放
        }

        /**
         * 将整数采样转换为浮点 (-1.0 到 1.0)
         * 使用内存池避免频繁分配
         */
        void AudioMixer::ConvertToFloat(const void* input, uint inputSize, float** output, uint* outputCount, const AudioDataInfo& info)
        {
            uint bytesPerSample = info.bits_per_sample / 8;
            *outputCount = inputSize / bytesPerSample;

            // 使用临时缓冲区
            temp_buffer.Ensure(*outputCount);
            *output = temp_buffer.Get();

            if(info.is_float && info.bits_per_sample == 32)
            {
                // 已经是float，直接复制
                const float* samples = (const float*)input;
                memcpy(*output, samples, (*outputCount) * sizeof(float));
            }
            else if(info.bits_per_sample == 32)
            {
                const int32_t* samples = (const int32_t*)input;
                for(uint i = 0; i < *outputCount; i++)
                {
                    // 转换为 -1.0 到 1.0 范围
                    (*output)[i] = samples[i] / 2147483648.0f;
                }
            }
            else if(info.bits_per_sample == 16)
            {
                const int16_t* samples = (const int16_t*)input;
                for(uint i = 0; i < *outputCount; i++)
                {
                    // 转换为 -1.0 到 1.0 范围
                    (*output)[i] = samples[i] / 32768.0f;
                }
            }
            else if(info.bits_per_sample == 8)
            {
                const int8_t* samples = (const int8_t*)input;
                for(uint i = 0; i < *outputCount; i++)
                {
                    // 转换为 -1.0 到 1.0 范围
                    (*output)[i] = samples[i] / 128.0f;
                }
            }
        }

        /**
         * 将浮点采样转换为整数格式
         */
        /**
         * 生成TPDF抖动噪声 (Triangular Probability Density Function)
         * TPDF使用两个均匀分布的随机数相加，产生三角形分布
         * 这种分布在听觉上比单个均匀分布更自然
         */
        float AudioMixer::GenerateTPDFDither()
        {
            // 生成两个[-1.0, 1.0]范围的随机数
            float r1 = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
            float r2 = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
            // 相加得到三角形分布
            return (r1 + r2) * 0.5f;
        }

        /**
         * 将浮点采样转换为目标格式
         * 支持float32/int32/int16/int8输出，int16转换时可选择使用TPDF抖动
         */
        void AudioMixer::ConvertFromFloat(const float* input, uint sampleCount, void** output, uint* outputSize, const AudioDataInfo& outputInfo)
        {
            if(outputInfo.is_float && outputInfo.bits_per_sample == 32)
            {
                // 输出float32
                *outputSize = sampleCount * sizeof(float);
                *output = new float[sampleCount];
                memcpy(*output, input, *outputSize);
            }
            else if(outputInfo.bits_per_sample == 32)
            {
                // 输出int32
                *outputSize = sampleCount * sizeof(int32_t);
                int32_t* samples = new int32_t[sampleCount];
                *output = samples;

                for(uint i = 0; i < sampleCount; i++)
                {
                    // 钳位到 -1.0 到 1.0
                    float sample = input[i];
                    if(sample > 1.0f) sample = 1.0f;
                    if(sample < -1.0f) sample = -1.0f;

                    samples[i] = (int32_t)((double)sample * 2147483647.0);
                }
            }
            else if(outputInfo.bits_per_sample == 16)
            {
                // 输出int16
                *outputSize = sampleCount * sizeof(int16_t);
                int16_t* samples = new int16_t[sampleCount];
                *output = samples;

                if(config.use_dither)
                {
                    // 使用TPDF抖动
                    LogInfo(OS_TEXT("Applying TPDF dither for float32->int16 conversion"));

                    for(uint i = 0; i < sampleCount; i++)
                    {
                        // 钳位到 -1.0 到 1.0
                        float sample = input[i];
                        if(sample > 1.0f) sample = 1.0f;
                        if(sample < -1.0f) sample = -1.0f;

                        // 添加TPDF抖动噪声（抖动幅度约为1 LSB）
                        float dither = GenerateTPDFDither() / 32768.0f;
                        sample += dither;

                        // 转换为int16
                        samples[i] = (int16_t)(sample * 32767.0f);
                    }
                }
                else
                {
                    // 不使用抖动，直接转换
                    for(uint i = 0; i < sampleCount; i++)
                    {
                        // 钳位到 -1.0 到 1.0
                        float sample = input[i];
                        if(sample > 1.0f) sample = 1.0f;
                        if(sample < -1.0f) sample = -1.0f;

                        // 转换为int16
                        samples[i] = (int16_t)(sample * 32767.0f);
                    }
                }
            }
            else if(outputInfo.bits_per_sample == 8)
            {
                // 输出int8
                *outputSize = sampleCount * sizeof(int8_t);
                int8_t* samples = new int8_t[sampleCount];
                *output = samples;

                for(uint i = 0; i < sampleCount; i++)
                {
                    // 钳位到 -1.0 到 1.0
                    float sample = input[i];
                    if(sample > 1.0f) sample = 1.0f;
                    if(sample < -1.0f) sample = -1.0f;

                    // 转换为int8
                    samples[i] = (int8_t)(sample * 127.0f);
                }
            }
        }

        /**
         * 添加音源数据
         */
        int AudioMixer::AddSourceAudio(const AudioDataInfo &info,const void *data)
        {
            if(!data || info.data_size == 0 || info.sample_rate == 0)
            {
                LogError(OS_TEXT("Invalid audio data parameters"));
                return -1;
            }

            if(info.channels==0||info.bits_per_sample==0)
            {
                LogError(OS_TEXT("Invalid audio format (channels/bits_per_sample)"));
                return -1;
            }

            if(!has_common_info)
            {
                common_info = info;
                has_common_info = true;
            }
            else
            {
                if(common_info.sample_rate != info.sample_rate ||
                   common_info.channels != info.channels ||
                   common_info.bits_per_sample != info.bits_per_sample ||
                   common_info.is_float != info.is_float)
                {
                    LogError(OS_TEXT("Source audio format/sample rate mismatch; unify before mixing"));
                    return -1;
                }
            }

            SourceAudio source;
            source.info = info;
            source.data = data;
            source.data_size = info.data_size;

            sources.Add(source);
            return sources.GetCount() - 1;
        }

        int AudioMixer::AddSourceAudio(const void *data,uint size,uint format,uint sample_rate)
        {
            AudioDataInfo info;

            if(!openal::FromOpenALFormat(format,info))
            {
                LogError(OS_TEXT("Unsupported audio format: ") + OSString::numberOf(format));
                return -1;
            }

            info.sample_rate = sample_rate;
            info.data_size = size;

            return AddSourceAudio(info,data);
        }

        /**
         * 清除所有音源
         */
        void AudioMixer::ClearSources()
        {
            sources.Clear();
            has_common_info = false;
        }

        /**
         * 添加混音轨道
         */
        void AudioMixer::AddTrack(const MixingTrack& track)
        {
            tracks.Add(track);
        }

        /**
         * 添加混音轨道(便捷方法)
         */
        void AudioMixer::AddTrack(uint source_index, float time_offset, float volume, float pitch)
        {
            AddTrack(MixingTrack(source_index, time_offset, volume, pitch));
        }

        /**
         * 清除所有轨道
         */
        void AudioMixer::ClearTracks()
        {
            tracks.Clear();
        }

        /**
         * 应用音调变化(简单的线性插值重采样) - float版本
         * 按帧处理，逐声道线性插值
         */
        void AudioMixer::ApplyPitchShift(const float* input, uint inputFrameCount, uint channels,
                                         float** output, uint* outputFrameCount, float pitch)
        {
            if(channels == 0)
                channels = 1;

            if(pitch < MinPitch || pitch > MaxPitch)
                pitch = DefaultPitch;

            // 如果音调不变，直接复制
            if(fabs(pitch - DefaultPitch) < 0.001f)
            {
                *outputFrameCount = inputFrameCount;
                *output = new float[inputFrameCount * channels];
                memcpy(*output, input, inputFrameCount * channels * sizeof(float));
                return;
            }

            // 计算输出帧数
            *outputFrameCount = (uint)(inputFrameCount / pitch);
            *output = new float[*outputFrameCount * channels];

            const uint inputSampleCount = inputFrameCount * channels;

            // 逐声道线性插值重采样
            for(uint ch = 0; ch < channels; ch++)
            {
                for(uint i = 0; i < *outputFrameCount; i++)
                {
                    float sourcePos = i * pitch;
                    uint sourceFrame = (uint)sourcePos;
                    float fraction = sourcePos - sourceFrame;

                    // 确保不会越界
                    if(sourceFrame >= inputFrameCount)
                        sourceFrame = inputFrameCount - 1;

                    uint idx0 = sourceFrame * channels + ch;
                    uint idx1 = idx0 + channels;

                    // 线性插值
                    float sample1 = input[idx0];
                    float sample2;

                    if(idx1 < inputSampleCount)
                        sample2 = input[idx1];
                    else
                        sample2 = sample1; // 最后一个采样，使用相同的值

                    (*output)[i * channels + ch] = sample1 * (1.0f - fraction) + sample2 * fraction;
                }
            }
        }

        /**
         * 软削波函数 - 使用tanh提供平滑的削波效果
         * tanh函数提供S形曲线，当输入接近±1时平滑压缩
         * 相比硬削波，软削波产生的失真更自然、更悦耳
         */
        float AudioMixer::SoftClip(float sample)
        {
            // tanh函数提供自然的软削波
            // 输入范围 (-∞, +∞)，输出范围 (-1.0, +1.0)
            // 当|sample| < 1.0时，几乎线性
            // 当|sample| > 1.0时，平滑压缩到±1.0
            return tanhf(sample);
        }

        /**
         * 应用软削波到float缓冲区
         */
        void AudioMixer::ApplySoftClipping(float* buffer, uint count)
        {
            for(uint i = 0; i < count; i++)
            {
                buffer[i] = SoftClip(buffer[i]);
            }
        }

        /**
         * 执行混音 - 完全使用float内部处理，使用内存池避免频繁分配
         */
        bool AudioMixer::Mix(void** outputData, uint* outputSize, float loopLength)
        {
            if(sources.GetCount() == 0)
            {
                LogError(OS_TEXT("No source audio data"));
                RETURN_FALSE;
            }

            if(tracks.GetCount() == 0)
            {
                LogError(OS_TEXT("No mixing tracks"));
                RETURN_FALSE;
            }

            if(!has_common_info)
            {
                LogError(OS_TEXT("No common audio format info"));
                RETURN_FALSE;
            }

            // 如果没有指定循环长度，计算所有轨道的最大时间
            if(loopLength <= 0.0f)
            {
                loopLength = 0.0f;
                for(auto track:tracks)
                {
                    if(track.source_index >= (uint)sources.GetCount())
                    {
                        LogError(OS_TEXT("Track source index out of range"));
                        RETURN_FALSE;
                    }

                    const SourceAudio& source = sources[track.source_index];
                    uint channels = source.info.channels;
                    if(channels == 0) channels = 1;

                    uint bytesPerSample = source.info.bits_per_sample / 8;
                    uint sourceFrameCount = (source.data_size / bytesPerSample) / channels;
                    float sourceDuration = (float)sourceFrameCount / source.info.sample_rate;
                    float trackEnd = track.time_offset + sourceDuration / track.pitch;

                    if(trackEnd > loopLength)
                        loopLength = trackEnd;
                }
            }

            const uint channels = common_info.channels ? common_info.channels : 1;

            // 计算输出帧数与采样数
            uint outputFrameCount = (uint)(loopLength * common_info.sample_rate);
            uint outputSampleCount = outputFrameCount * channels;

            // 确保缓冲区足够大
            pool_buffer.Ensure(outputSampleCount);

            LogInfo(OS_TEXT("Mixing ") + OSString::numberOf(tracks.GetCount()) +
                    OS_TEXT(" tracks, output duration: ") + OSString::floatOf(loopLength,3) +
                    OS_TEXT(" seconds, output sample rate: ") + OSString::numberOf((int)common_info.sample_rate) +
                    OS_TEXT(", output channels: ") + OSString::numberOf((int)channels) +
                    OS_TEXT(", output format: ") + (common_info.is_float ? OS_TEXT("float32") : OS_TEXT("int")) +
                    OS_TEXT(", pool buffer size: ") + OSString::numberOf((int)pool_buffer.GetSize()) + OS_TEXT(" samples"));

            // 使用池缓冲区进行混音
            float* mixBuffer = pool_buffer.Get();
            memset(mixBuffer, 0, outputSampleCount * sizeof(float));

            // 为每个轨道应用变换并混音
            for(auto track:tracks)
            {
                if(track.source_index >= (uint)sources.GetCount())
                {
                    LogError(OS_TEXT("Track source index out of range"));
                    RETURN_FALSE;
                }

                const SourceAudio& source = sources[track.source_index];

                // 将源数据转换为float（使用临时缓冲区）
                float* sourceFloat = nullptr;
                uint sourceFloatCount = 0;
                ConvertToFloat(source.data, source.data_size, &sourceFloat, &sourceFloatCount, source.info);

                // 应用音调变化（按帧处理）
                uint sourceFrameCount = sourceFloatCount / channels;
                float* pitchShiftedData = nullptr;
                uint pitchShiftedFrameCount = 0;
                ApplyPitchShift(sourceFloat, sourceFrameCount, channels,
                              &pitchShiftedData, &pitchShiftedFrameCount, track.pitch);

                uint pitchShiftedSampleCount = pitchShiftedFrameCount * channels;

                // 计算起始采样位置
                uint startFrame = (uint)(track.time_offset * common_info.sample_rate);
                uint startSample = startFrame * channels;

                // 混合到输出 (float混音，无需担心溢出)
                for(uint i = 0; i < pitchShiftedSampleCount && (startSample + i) < outputSampleCount; i++)
                {
                    // 应用音量并混合 - float混音非常简单
                    mixBuffer[startSample + i] += pitchShiftedData[i] * track.volume * config.master_volume;
                }

                delete[] pitchShiftedData;
            }

            // 应用软削波或归一化
            if(config.use_soft_clipper)
            {
                // 使用软削波处理越界数据
                LogInfo(OS_TEXT("Applying soft clipping (tanh)"));
                ApplySoftClipping(mixBuffer, outputSampleCount);
            }
            else if(config.normalize)
            {
                // 使用传统归一化（硬削波）
                // 查找峰值
                float peak = 0.0f;
                for(uint i = 0; i < outputSampleCount; i++)
                {
                    float abs_sample = fabs(mixBuffer[i]);
                    if(abs_sample > peak)
                        peak = abs_sample;
                }

                // 如果峰值超过1.0，进行归一化
                if(peak > 1.0f)
                {
                    float normFactor = 1.0f / peak;
                    LogInfo(OS_TEXT("Normalizing audio, peak: ") + OSString::floatOf(peak,3) +
                           OS_TEXT(", factor: ") + OSString::floatOf(normFactor,3));

                    for(uint i = 0; i < outputSampleCount; i++)
                    {
                        mixBuffer[i] *= normFactor;
                    }
                }
            }
            // 如果都不启用，则可能存在超出[-1.0, 1.0]的数据，在转换时会被硬削波

            // 转换为目标格式（注意：不再使用mixBuffer，因为ConvertFromFloat会分配新内存）
            const AudioDataInfo &outputInfo = output_format;

            if(!outputInfo.is_float && outputInfo.channels != channels)
            {
                LogError(OS_TEXT("Output format channel count must match source channel count"));
                RETURN_FALSE;
            }

            ConvertFromFloat(mixBuffer, outputSampleCount, outputData, outputSize, outputInfo);

            // 不再删除mixBuffer，因为它是池缓冲区

            LogInfo(OS_TEXT("Mixing completed successfully"));
            return(true);
        }

}//namespace hgl::audio
