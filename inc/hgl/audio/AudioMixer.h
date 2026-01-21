#pragma once

#include<hgl/audio/AudioMixerTypes.h>
#include<hgl/audio/AudioBuffer.h>
#include<hgl/type/ObjectList.h>
#include<hgl/log/Log.h>

namespace hgl
{
    namespace audio
    {
        /**
         * 音频混音器
         * 用于将多个音轨叠加混音成一个新的音频数据
         * 仅支持单声道音频，支持时间偏移、音量调整、音调变化、采样率转换等变换
         */
        class AudioMixer
        {
            OBJECT_LOGGER
            
        private:
            
            // Pitch shifting constants
            static constexpr float MIN_PITCH = 0.5f;
            static constexpr float MAX_PITCH = 2.0f;
            static constexpr float DEFAULT_PITCH = 1.0f;
            
            ObjectList<MixingTrack> tracks;         ///< 混音轨道列表
            MixerConfig config;                     ///< 混音器配置
            
            AudioDataInfo sourceInfo;               ///< 源音频数据信息
            const void* sourceData;                 ///< 源音频数据指针
            uint sourceDataSize;                    ///< 源音频数据大小
            uint outputSampleRate;                  ///< 输出采样率 (0=使用源采样率)
            uint outputFormat;                      ///< 输出格式 (AL_FORMAT_MONO16 或 AL_FORMAT_MONO_FLOAT32)
            
            /**
             * 获取音频格式信息
             */
            bool ParseAudioFormat(uint format, AudioDataInfo& info);
            
            /**
             * 将整数采样转换为浮点 (-1.0 到 1.0)
             */
            void ConvertToFloat(const void* input, uint inputSize, float** output, uint* outputCount, const AudioDataInfo& info);
            
            /**
             * 将浮点采样转换为整数格式
             */
            void ConvertFromFloat(const float* input, uint sampleCount, void** output, uint* outputSize, uint targetFormat);
            
            /**
             * 应用简单的音调变化(线性插值重采样) - float版本
             */
            void ApplyPitchShift(const float* input, uint inputCount, 
                               float** output, uint* outputCount, float pitch);
            
            /**
             * 应用采样率转换 - float版本
             */
            void Resample(const float* input, uint inputCount, uint inputSampleRate,
                         float** output, uint* outputCount, uint outputSampleRate);
            
            /**
             * 软削波函数 - 使用tanh提供平滑的削波效果
             * 将超出[-1.0, 1.0]范围的信号平滑压缩，避免硬削波的刺耳失真
             * @param sample 输入采样值
             * @return 削波后的采样值
             */
            static float SoftClip(float sample);
            
            /**
             * 应用软削波到float缓冲区
             * @param buffer float采样缓冲区
             * @param count 采样数量
             */
            static void ApplySoftClipping(float* buffer, uint count);
            
            /**
             * 生成TPDF抖动噪声 (Triangular Probability Density Function)
             * TPDF抖动是音频处理中最常用的抖动类型，提供平滑自然的量化噪声掩蔽
             * @return 范围在[-1.0, 1.0]的随机噪声值
             */
            static float GenerateTPDFDither();
            
        public:
            
            AudioMixer();
            virtual ~AudioMixer();
            
            /**
             * 设置源音频数据
             * @param data 音频数据指针
             * @param size 数据大小
             * @param format 音频格式 (AL_FORMAT_*)
             * @param sampleRate 采样率
             */
            bool SetSourceAudio(const void* data, uint size, uint format, uint sampleRate);
            
            /**
             * 从AudioBuffer设置源音频数据
             */
            bool SetSourceAudio(AudioBuffer* buffer);
            
            /**
             * 添加混音轨道
             * @param track 轨道参数
             */
            void AddTrack(const MixingTrack& track);
            
            /**
             * 添加混音轨道(便捷方法)
             * @param timeOffset 时间偏移(秒)
             * @param volume 音量(0.0-1.0)
             * @param pitch 音调(0.5-2.0)
             */
            void AddTrack(float timeOffset, float volume, float pitch);
            
            /**
             * 清除所有轨道
             */
            void ClearTracks();
            
            /**
             * 获取轨道数量
             */
            int GetTrackCount() const { return tracks.GetCount(); }
            
            /**
             * 设置混音器配置
             */
            void SetConfig(const MixerConfig& cfg) { config = cfg; }
            
            /**
             * 获取混音器配置
             */
            const MixerConfig& GetConfig() const { return config; }
            
            /**
             * 设置输出采样率
             * @param sampleRate 输出采样率 (如44100, 48000), 0表示使用源采样率
             */
            void SetOutputSampleRate(uint sampleRate) { outputSampleRate = sampleRate; }
            
            /**
             * 设置输出格式
             * @param format 输出格式 (AL_FORMAT_MONO16 或 AL_FORMAT_MONO_FLOAT32)
             */
            void SetOutputFormat(uint format) { outputFormat = format; }
            
            /**
             * 获取输出采样率
             */
            uint GetOutputSampleRate() const { return outputSampleRate > 0 ? outputSampleRate : sourceInfo.sampleRate; }
            
            /**
             * 获取输出格式
             */
            uint GetOutputFormat() const { return outputFormat; }
            
            /**
             * 执行混音
             * @param outputData 输出数据指针(需要调用者释放)
             * @param outputSize 输出数据大小
             * @param loopLength 循环长度(秒), 如果为0则自动计算
             * @return 是否成功
             */
            bool Mix(void** outputData, uint* outputSize, float loopLength = 0.0f);
            
            /**
             * 获取输出音频信息
             */
            const AudioDataInfo& GetOutputInfo() const { return sourceInfo; }
        };
        
    }//namespace audio
}//namespace hgl
