#include<hgl/audio/AudioMixer.h>
#include<hgl/audio/OpenAL.h>
#include<hgl/type/Smart.h>
#include<math.h>
#include<string.h>

using namespace openal;

namespace hgl
{
    namespace audio
    {
        AudioMixer::AudioMixer()
            : poolBuffer(OS_TEXT("AudioMixer::poolBuffer")),
              tempBuffer(OS_TEXT("AudioMixer::tempBuffer"))
        {
            hasCommonInfo = false;
            outputFormat = AL_FORMAT_MONO16;  // 默认输出int16
        }

        AudioMixer::~AudioMixer()
        {
            ClearTracks();
            // 内存池自动释放
        }

        /**
         * 解析音频格式 - 仅支持单声道
         */
        bool AudioMixer::ParseAudioFormat(uint format, AudioDataInfo& info)
        {
            info.format = format;
            info.channels = 1;  // 仅支持单声道
            info.isFloat = false;

            switch(format)
            {
                case AL_FORMAT_MONO8:
                    info.bitsPerSample = 8;
                    break;

                case AL_FORMAT_MONO16:
                    info.bitsPerSample = 16;
                    break;

                case AL_FORMAT_MONO_FLOAT32:
                    info.bitsPerSample = 32;
                    info.isFloat = true;
                    break;

                default:
                    LogError(OS_TEXT("Unsupported audio format (only mono supported): ") + OSString::numberOf(format));
                    RETURN_FALSE;
            }

            return(true);
        }

        /**
         * 将整数采样转换为浮点 (-1.0 到 1.0)
         * 使用内存池避免频繁分配
         */
        void AudioMixer::ConvertToFloat(const void* input, uint inputSize, float** output, uint* outputCount, const AudioDataInfo& info)
        {
            uint bytesPerSample = info.bitsPerSample / 8;
            *outputCount = inputSize / bytesPerSample;

            // 使用临时缓冲区
            tempBuffer.Ensure(*outputCount);
            *output = tempBuffer.Get();

            if(info.bitsPerSample == 16)
            {
                const int16_t* samples = (const int16_t*)input;
                for(uint i = 0; i < *outputCount; i++)
                {
                    // 转换为 -1.0 到 1.0 范围
                    (*output)[i] = samples[i] / 32768.0f;
                }
            }
            else if(info.bitsPerSample == 8)
            {
                const int8_t* samples = (const int8_t*)input;
                for(uint i = 0; i < *outputCount; i++)
                {
                    // 转换为 -1.0 到 1.0 范围
                    (*output)[i] = samples[i] / 128.0f;
                }
            }
            else if(info.isFloat && info.bitsPerSample == 32)
            {
                // 已经是float，直接复制
                const float* samples = (const float*)input;
                memcpy(*output, samples, (*outputCount) * sizeof(float));
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
         * 支持float32和int16输出，int16转换时可选择使用TPDF抖动
         */
        void AudioMixer::ConvertFromFloat(const float* input, uint sampleCount, void** output, uint* outputSize, uint targetFormat)
        {
            if(targetFormat == AL_FORMAT_MONO_FLOAT32)
            {
                // 输出float32
                *outputSize = sampleCount * sizeof(float);
                *output = new float[sampleCount];
                memcpy(*output, input, *outputSize);
            }
            else if(targetFormat == AL_FORMAT_MONO16)
            {
                // 输出int16
                *outputSize = sampleCount * sizeof(int16_t);
                int16_t* samples = new int16_t[sampleCount];
                *output = samples;

                if(config.useDither)
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
            else if(targetFormat == AL_FORMAT_MONO8)
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
        int AudioMixer::AddSourceAudio(const void* data, uint size, uint format, uint sampleRate)
        {
            if(!data || size == 0 || sampleRate == 0)
            {
                LogError(OS_TEXT("Invalid audio data parameters"));
                return -1;
            }

            AudioDataInfo info;
            if(!ParseAudioFormat(format, info))
            {
                return -1;
            }

            info.sampleRate = sampleRate;
            info.dataSize = size;

            if(!hasCommonInfo)
            {
                commonInfo = info;
                hasCommonInfo = true;
            }
            else
            {
                if(commonInfo.format != info.format ||
                   commonInfo.sampleRate != info.sampleRate ||
                   commonInfo.bitsPerSample != info.bitsPerSample ||
                   commonInfo.isFloat != info.isFloat)
                {
                    LogError(OS_TEXT("Source audio format/sample rate mismatch; unify before mixing"));
                    return -1;
                }
            }

            SourceAudio source;
            source.info = info;
            source.data = data;
            source.dataSize = size;

            sources.Add(source);
            return sources.GetCount() - 1;
        }

        /**
         * 清除所有音源
         */
        void AudioMixer::ClearSources()
        {
            sources.Clear();
            hasCommonInfo = false;
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
        void AudioMixer::AddTrack(uint sourceIndex, float timeOffset, float volume, float pitch)
        {
            AddTrack(MixingTrack(sourceIndex, timeOffset, volume, pitch));
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
         */
        void AudioMixer::ApplyPitchShift(const float* input, uint inputCount,
                                         float** output, uint* outputCount, float pitch)
        {
            if(pitch < MinPitch || pitch > MaxPitch)
                pitch = DefaultPitch;

            // 如果音调不变，直接复制
            if(fabs(pitch - DefaultPitch) < 0.001f)
            {
                *outputCount = inputCount;
                *output = new float[inputCount];
                memcpy(*output, input, inputCount * sizeof(float));
                return;
            }

            // 计算输出大小
            *outputCount = (uint)(inputCount / pitch);
            *output = new float[*outputCount];

            // 线性插值重采样
            for(uint i = 0; i < *outputCount; i++)
            {
                float sourcePos = i * pitch;
                uint sourceIndex = (uint)sourcePos;
                float fraction = sourcePos - sourceIndex;

                // 确保不会越界
                if(sourceIndex >= inputCount)
                    sourceIndex = inputCount - 1;

                // 线性插值
                float sample1 = input[sourceIndex];
                float sample2;

                if(sourceIndex + 1 < inputCount)
                    sample2 = input[sourceIndex + 1];
                else
                    sample2 = sample1; // 最后一个采样，使用相同的值

                (*output)[i] = sample1 * (1.0f - fraction) + sample2 * fraction;
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

            if(!hasCommonInfo)
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
                    if(track.sourceIndex >= (uint)sources.GetCount())
                    {
                        LogError(OS_TEXT("Track source index out of range"));
                        RETURN_FALSE;
                    }

                    const SourceAudio& source = sources[track.sourceIndex];
                    uint bytesPerSample = source.info.bitsPerSample / 8;
                    uint sourceSampleCount = source.dataSize / bytesPerSample;
                    float sourceDuration = (float)sourceSampleCount / source.info.sampleRate;
                    float trackEnd = track.timeOffset + sourceDuration / track.pitch;

                    if(trackEnd > loopLength)
                        loopLength = trackEnd;
                }
            }

            // 计算输出采样数
            uint outputSampleCount = (uint)(loopLength * commonInfo.sampleRate);

            // 预分配2倍大小的缓冲区以减少动态分配（基于用户要求）
            poolBuffer.Preallocate(outputSampleCount, 2.0f);

            LogInfo(OS_TEXT("Mixing ") + OSString::numberOf(tracks.GetCount()) +
                    OS_TEXT(" tracks, output duration: ") + OSString::floatOf(loopLength,3) +
                    OS_TEXT(" seconds, output sample rate: ") + OSString::numberOf((int)commonInfo.sampleRate) +
                    OS_TEXT(", output format: ") + (outputFormat == AL_FORMAT_MONO_FLOAT32 ? OS_TEXT("float32") : OS_TEXT("int16")) +
                    OS_TEXT(", pool buffer size: ") + OSString::numberOf((int)poolBuffer.GetSize()) + OS_TEXT(" samples"));

            // 使用池缓冲区进行混音
            float* mixBuffer = poolBuffer.Get();
            memset(mixBuffer, 0, outputSampleCount * sizeof(float));

            // 为每个轨道应用变换并混音
            for(auto track:tracks)
            {
                if(track.sourceIndex >= (uint)sources.GetCount())
                {
                    LogError(OS_TEXT("Track source index out of range"));
                    RETURN_FALSE;
                }

                const SourceAudio& source = sources[track.sourceIndex];

                // 将源数据转换为float（使用临时缓冲区）
                float* sourceFloat = nullptr;
                uint sourceFloatCount = 0;
                ConvertToFloat(source.data, source.dataSize, &sourceFloat, &sourceFloatCount, source.info);

                // 应用音调变化
                float* pitchShiftedData = nullptr;
                uint pitchShiftedCount = 0;
                ApplyPitchShift(sourceFloat, sourceFloatCount,
                              &pitchShiftedData, &pitchShiftedCount, track.pitch);

                // 计算起始位置
                uint startSample = (uint)(track.timeOffset * commonInfo.sampleRate);

                // 混合到输出 (float混音，无需担心溢出)
                for(uint i = 0; i < pitchShiftedCount && (startSample + i) < outputSampleCount; i++)
                {
                    // 应用音量并混合 - float混音非常简单
                    mixBuffer[startSample + i] += pitchShiftedData[i] * track.volume * config.masterVolume;
                }

                delete[] pitchShiftedData;
            }

            // 应用软削波或归一化
            if(config.useSoftClipper)
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
            ConvertFromFloat(mixBuffer, outputSampleCount, outputData, outputSize, outputFormat);

            // 不再删除mixBuffer，因为它是池缓冲区

            LogInfo(OS_TEXT("Mixing completed successfully"));
            return(true);
        }

    }//namespace audio
}//namespace hgl
