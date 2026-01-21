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
        {
            sourceData = nullptr;
            sourceDataSize = 0;
            outputSampleRate = 0;  // 0表示使用源采样率
            outputFormat = AL_FORMAT_MONO16;  // 默认输出int16
        }
        
        AudioMixer::~AudioMixer()
        {
            ClearTracks();
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
            
            RETURN_TRUE;
        }
        
        /**
         * 将整数采样转换为浮点 (-1.0 到 1.0)
         */
        void AudioMixer::ConvertToFloat(const void* input, uint inputSize, float** output, uint* outputCount, const AudioDataInfo& info)
        {
            uint bytesPerSample = info.bitsPerSample / 8;
            *outputCount = inputSize / bytesPerSample;
            *output = new float[*outputCount];
            
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
         * 设置源音频数据
         */
        bool AudioMixer::SetSourceAudio(const void* data, uint size, uint format, uint sampleRate)
        {
            if(!data || size == 0 || sampleRate == 0)
            {
                LogError(OS_TEXT("Invalid audio data parameters"));
                RETURN_FALSE;
            }
            
            if(!ParseAudioFormat(format, sourceInfo))
            {
                RETURN_FALSE;
            }
            
            sourceData = data;
            sourceDataSize = size;
            sourceInfo.sampleRate = sampleRate;
            sourceInfo.dataSize = size;
            
            RETURN_TRUE;
        }
        
        /**
         * 从AudioBuffer设置源音频数据
         */
        bool AudioMixer::SetSourceAudio(AudioBuffer* buffer)
        {
            if(!buffer)
            {
                LogError(OS_TEXT("AudioBuffer is null"));
                RETURN_FALSE;
            }
            
            // 注意: AudioBuffer没有直接提供数据访问接口
            // 这里只是一个示例，实际使用时需要AudioBuffer提供数据访问方法
            LogError(OS_TEXT("SetSourceAudio from AudioBuffer not fully implemented - need data access"));
            RETURN_FALSE;
        }
        
        /**
         * 添加混音轨道
         */
        void AudioMixer::AddTrack(const MixingTrack& track)
        {
            tracks.Add(new MixingTrack(track));
        }
        
        /**
         * 添加混音轨道(便捷方法)
         */
        void AudioMixer::AddTrack(float timeOffset, float volume, float pitch)
        {
            MixingTrack track(timeOffset, volume, pitch);
            AddTrack(track);
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
            if(pitch < MIN_PITCH || pitch > MAX_PITCH)
                pitch = DEFAULT_PITCH;
            
            // 如果音调不变，直接复制
            if(fabs(pitch - DEFAULT_PITCH) < 0.001f)
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
         * 应用采样率转换(线性插值重采样) - float版本
         */
        void AudioMixer::Resample(const float* input, uint inputCount, uint inputSampleRate,
                                 float** output, uint* outputCount, uint outputSampleRate)
        {
            if(inputSampleRate == outputSampleRate || outputSampleRate == 0)
            {
                // 采样率相同，直接复制
                *outputCount = inputCount;
                *output = new float[inputCount];
                memcpy(*output, input, inputCount * sizeof(float));
                return;
            }
            
            // 计算重采样比例
            float resampleRatio = (float)outputSampleRate / (float)inputSampleRate;
            
            *outputCount = (uint)(inputCount * resampleRatio);
            *output = new float[*outputCount];
            
            // 线性插值重采样
            for(uint i = 0; i < *outputCount; i++)
            {
                float sourcePos = i / resampleRatio;
                uint sourceIndex = (uint)sourcePos;
                float fraction = sourcePos - sourceIndex;
                
                if(sourceIndex >= inputCount)
                    sourceIndex = inputCount - 1;
                
                // 线性插值
                float sample1 = input[sourceIndex];
                float sample2;
                
                if(sourceIndex + 1 < inputCount)
                    sample2 = input[sourceIndex + 1];
                else
                    sample2 = sample1;
                    
                (*output)[i] = sample1 * (1.0f - fraction) + sample2 * fraction;
            }
        }
        
        /**
         * 执行混音 - 完全使用float内部处理
         */
        bool AudioMixer::Mix(void** outputData, uint* outputSize, float loopLength)
        {
            if(!sourceData || sourceDataSize == 0)
            {
                LogError(OS_TEXT("No source audio data"));
                RETURN_FALSE;
            }
            
            if(tracks.GetCount() == 0)
            {
                LogError(OS_TEXT("No mixing tracks"));
                RETURN_FALSE;
            }
            
            // 计算循环长度
            uint bytesPerSample = sourceInfo.bitsPerSample / 8;
            uint sourceSampleCount = sourceDataSize / bytesPerSample;
            float sourceDuration = (float)sourceSampleCount / sourceInfo.sampleRate;
            
            // 如果没有指定循环长度，计算所有轨道的最大时间
            if(loopLength <= 0.0f)
            {
                loopLength = sourceDuration;
                for(int i = 0; i < tracks.GetCount(); i++)
                {
                    MixingTrack* track = tracks[i];
                    float trackEnd = track->timeOffset + sourceDuration / track->pitch;
                    if(trackEnd > loopLength)
                        loopLength = trackEnd;
                }
            }
            
            // 确定最终输出采样率
            uint finalSampleRate = outputSampleRate > 0 ? outputSampleRate : sourceInfo.sampleRate;
            
            // 计算输出采样数
            uint outputSampleCount = (uint)(loopLength * finalSampleRate);
            
            LogInfo(OS_TEXT("Mixing ") + OSString::numberOf(tracks.GetCount()) + 
                   OS_TEXT(" tracks, output duration: ") + OSString::floatOf(loopLength) + 
                   OS_TEXT(" seconds, output sample rate: ") + OSString::numberOf((int)finalSampleRate) +
                   OS_TEXT(", output format: ") + (outputFormat == AL_FORMAT_MONO_FLOAT32 ? OS_TEXT("float32") : OS_TEXT("int16")));
            
            // 创建float缓冲区用于混音
            float* mixBuffer = new float[outputSampleCount];
            memset(mixBuffer, 0, outputSampleCount * sizeof(float));
            
            // 将源数据转换为float
            float* sourceFloat = nullptr;
            uint sourceFloatCount = 0;
            ConvertToFloat(sourceData, sourceDataSize, &sourceFloat, &sourceFloatCount, sourceInfo);
            
            // 为每个轨道应用变换并混音
            for(int trackIndex = 0; trackIndex < tracks.GetCount(); trackIndex++)
            {
                MixingTrack* track = tracks[trackIndex];
                
                // 应用音调变化
                float* pitchShiftedData = nullptr;
                uint pitchShiftedCount = 0;
                ApplyPitchShift(sourceFloat, sourceFloatCount, 
                              &pitchShiftedData, &pitchShiftedCount, track->pitch);
                
                // 如果需要，应用采样率转换
                float* resampledData = nullptr;
                uint resampledCount = 0;
                if(finalSampleRate != sourceInfo.sampleRate)
                {
                    Resample(pitchShiftedData, pitchShiftedCount, sourceInfo.sampleRate,
                            &resampledData, &resampledCount, finalSampleRate);
                    delete[] pitchShiftedData;
                    pitchShiftedData = resampledData;
                    pitchShiftedCount = resampledCount;
                }
                
                // 计算起始位置
                uint startSample = (uint)(track->timeOffset * finalSampleRate);
                
                // 混合到输出 (float混音，无需担心溢出)
                for(uint i = 0; i < pitchShiftedCount && (startSample + i) < outputSampleCount; i++)
                {
                    // 应用音量并混合 - float混音非常简单
                    mixBuffer[startSample + i] += pitchShiftedData[i] * track->volume * config.masterVolume;
                }
                
                delete[] pitchShiftedData;
            }
            
            delete[] sourceFloat;
            
            // 可选：应用归一化
            if(config.normalize)
            {
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
                    LogInfo(OS_TEXT("Normalizing audio, peak: ") + OSString::floatOf(peak) + 
                           OS_TEXT(", factor: ") + OSString::floatOf(normFactor));
                    for(uint i = 0; i < outputSampleCount; i++)
                    {
                        mixBuffer[i] *= normFactor;
                    }
                }
            }
            
            // 转换为目标格式
            ConvertFromFloat(mixBuffer, outputSampleCount, outputData, outputSize, outputFormat);
            
            delete[] mixBuffer;
            
            LogInfo(OS_TEXT("Mixing completed successfully"));
            RETURN_TRUE;
        }
        
    }//namespace audio
}//namespace hgl
