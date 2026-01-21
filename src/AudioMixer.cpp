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
        }
        
        AudioMixer::~AudioMixer()
        {
            ClearTracks();
        }
        
        /**
         * 解析音频格式
         */
        bool AudioMixer::ParseAudioFormat(uint format, AudioDataInfo& info)
        {
            info.format = format;
            
            switch(format)
            {
                case AL_FORMAT_MONO8:
                    info.channels = 1;
                    info.bitsPerSample = 8;
                    break;
                    
                case AL_FORMAT_MONO16:
                    info.channels = 1;
                    info.bitsPerSample = 16;
                    break;
                    
                case AL_FORMAT_STEREO8:
                    info.channels = 2;
                    info.bitsPerSample = 8;
                    break;
                    
                case AL_FORMAT_STEREO16:
                    info.channels = 2;
                    info.bitsPerSample = 16;
                    break;
                    
                default:
                    LogError(OS_TEXT("Unsupported audio format: ") + OSString::numberOf(format));
                    RETURN_FALSE;
            }
            
            RETURN_TRUE;
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
         * 应用音调变化(简单的线性插值重采样)
         */
        void AudioMixer::ApplyPitchShift(const void* input, uint inputSize, 
                                         void** output, uint* outputSize,
                                         float pitch, const AudioDataInfo& info)
        {
            if(pitch < MIN_PITCH || pitch > MAX_PITCH)
                pitch = DEFAULT_PITCH;
            
            // 如果音调不变，直接复制
            if(fabs(pitch - DEFAULT_PITCH) < 0.001f)
            {
                *outputSize = inputSize;
                *output = new char[inputSize];
                memcpy(*output, input, inputSize);
                return;
            }
            
            // 计算输出大小
            uint sampleCount = inputSize / (info.bitsPerSample / 8) / info.channels;
            
            // 验证输入不为空
            if(sampleCount == 0)
            {
                *outputSize = 0;
                *output = nullptr;
                return;
            }
            
            uint outputSampleCount = (uint)(sampleCount / pitch);
            *outputSize = outputSampleCount * (info.bitsPerSample / 8) * info.channels;
            *output = new char[*outputSize];
            
            // 根据位深进行重采样
            if(info.bitsPerSample == 16)
            {
                const int16_t* inputSamples = (const int16_t*)input;
                int16_t* outputSamples = (int16_t*)*output;
                
                for(uint i = 0; i < outputSampleCount * info.channels; i++)
                {
                    float sourcePos = i * pitch;
                    uint sourceIndex = (uint)sourcePos;
                    float fraction = sourcePos - sourceIndex;
                    
                    // 确保不会越界
                    if(sourceIndex >= sampleCount * info.channels)
                        sourceIndex = sampleCount * info.channels - 1;
                    
                    // 线性插值
                    int16_t sample1 = inputSamples[sourceIndex];
                    int16_t sample2;
                    
                    if(sourceIndex + 1 < sampleCount * info.channels)
                        sample2 = inputSamples[sourceIndex + 1];
                    else
                        sample2 = sample1; // 最后一个采样，使用相同的值
                        
                    outputSamples[i] = (int16_t)(sample1 * (1.0f - fraction) + sample2 * fraction);
                }
            }
            else if(info.bitsPerSample == 8)
            {
                const int8_t* inputSamples = (const int8_t*)input;
                int8_t* outputSamples = (int8_t*)*output;
                
                for(uint i = 0; i < outputSampleCount * info.channels; i++)
                {
                    float sourcePos = i * pitch;
                    uint sourceIndex = (uint)sourcePos;
                    float fraction = sourcePos - sourceIndex;
                    
                    // 确保不会越界
                    if(sourceIndex >= sampleCount * info.channels)
                        sourceIndex = sampleCount * info.channels - 1;
                    
                    // 线性插值
                    int8_t sample1 = inputSamples[sourceIndex];
                    int8_t sample2;
                    
                    if(sourceIndex + 1 < sampleCount * info.channels)
                        sample2 = inputSamples[sourceIndex + 1];
                    else
                        sample2 = sample1; // 最后一个采样，使用相同的值
                        
                    outputSamples[i] = (int8_t)(sample1 * (1.0f - fraction) + sample2 * fraction);
                }
            }
        }
        
        /**
         * 执行混音
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
            uint bytesPerFrame = bytesPerSample * sourceInfo.channels;
            uint sourceSampleCount = sourceDataSize / bytesPerFrame;
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
            
            // 计算输出采样数
            uint outputSampleCount = (uint)(loopLength * sourceInfo.sampleRate);
            *outputSize = outputSampleCount * bytesPerFrame;
            *outputData = new char[*outputSize];
            memset(*outputData, 0, *outputSize);
            
            LogInfo(OS_TEXT("Mixing ") + OSString::numberOf(tracks.GetCount()) + 
                   OS_TEXT(" tracks, output duration: ") + OSString::floatOf(loopLength) + 
                   OS_TEXT(" seconds"));
            
            // 为每个轨道应用变换并混音
            for(int trackIndex = 0; trackIndex < tracks.GetCount(); trackIndex++)
            {
                MixingTrack* track = tracks[trackIndex];
                
                // 应用音调变化
                void* pitchShiftedData = nullptr;
                uint pitchShiftedSize = 0;
                ApplyPitchShift(sourceData, sourceDataSize, 
                              &pitchShiftedData, &pitchShiftedSize,
                              track->pitch, sourceInfo);
                
                // 计算起始位置
                uint startSample = (uint)(track->timeOffset * sourceInfo.sampleRate);
                uint trackSampleCount = pitchShiftedSize / bytesPerFrame;
                
                // 混合到输出
                if(sourceInfo.bitsPerSample == 16)
                {
                    int16_t* outputSamples = (int16_t*)*outputData;
                    const int16_t* trackSamples = (const int16_t*)pitchShiftedData;
                    
                    for(uint i = 0; i < trackSampleCount && (startSample + i) < outputSampleCount; i++)
                    {
                        uint outputBaseIndex = (startSample + i) * sourceInfo.channels;
                        uint inputBaseIndex = i * sourceInfo.channels;
                        
                        for(uint ch = 0; ch < sourceInfo.channels; ch++)
                        {
                            uint outputSampleIndex = outputBaseIndex + ch;
                            uint inputSampleIndex = inputBaseIndex + ch;
                            
                            // 应用音量并混合
                            int32_t sample = trackSamples[inputSampleIndex];
                            sample = (int32_t)(sample * track->volume * config.masterVolume);
                            
                            int32_t mixed = outputSamples[outputSampleIndex] + sample;
                            
                            // 限幅
                            if(mixed > 32767) mixed = 32767;
                            if(mixed < -32768) mixed = -32768;
                            
                            outputSamples[outputSampleIndex] = (int16_t)mixed;
                        }
                    }
                }
                else if(sourceInfo.bitsPerSample == 8)
                {
                    int8_t* outputSamples = (int8_t*)*outputData;
                    const int8_t* trackSamples = (const int8_t*)pitchShiftedData;
                    
                    for(uint i = 0; i < trackSampleCount && (startSample + i) < outputSampleCount; i++)
                    {
                        uint outputBaseIndex = (startSample + i) * sourceInfo.channels;
                        uint inputBaseIndex = i * sourceInfo.channels;
                        
                        for(uint ch = 0; ch < sourceInfo.channels; ch++)
                        {
                            uint outputSampleIndex = outputBaseIndex + ch;
                            uint inputSampleIndex = inputBaseIndex + ch;
                            
                            // 应用音量并混合
                            int16_t sample = trackSamples[inputSampleIndex];
                            sample = (int16_t)(sample * track->volume * config.masterVolume);
                            
                            int16_t mixed = outputSamples[outputSampleIndex] + sample;
                            
                            // 限幅
                            if(mixed > 127) mixed = 127;
                            if(mixed < -128) mixed = -128;
                            
                            outputSamples[outputSampleIndex] = (int8_t)mixed;
                        }
                    }
                }
                
                delete[] (char*)pitchShiftedData;
            }
            
            LogInfo(OS_TEXT("Mixing completed successfully"));
            RETURN_TRUE;
        }
        
    }//namespace audio
}//namespace hgl
