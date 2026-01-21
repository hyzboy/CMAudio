#pragma once

#include<hgl/audio/AudioMixerTypes.h>
#include<hgl/audio/AudioBuffer.h>
#include<hgl/type/List.h>
#include<hgl/log/Log.h>

namespace hgl
{
    namespace audio
    {
        /**
         * 音频混音器
         * 用于将多个音轨叠加混音成一个新的音频数据
         * 支持时间偏移、音量调整、音调变化等变换
         */
        class AudioMixer
        {
            OBJECT_LOGGER
            
        private:
            
            ObjectList<MixingTrack> tracks;         ///< 混音轨道列表
            MixerConfig config;                     ///< 混音器配置
            
            AudioDataInfo sourceInfo;               ///< 源音频数据信息
            const void* sourceData;                 ///< 源音频数据指针
            uint sourceDataSize;                    ///< 源音频数据大小
            
            /**
             * 获取音频格式信息
             */
            bool ParseAudioFormat(uint format, AudioDataInfo& info);
            
            /**
             * 应用简单的音调变化(线性插值重采样)
             */
            void ApplyPitchShift(const void* input, uint inputSize, 
                               void** output, uint* outputSize,
                               float pitch, const AudioDataInfo& info);
            
            /**
             * 混合多个采样点
             */
            int16_t MixSamples(int16_t* samples, int count, bool normalize);
            int8_t MixSamples8(int8_t* samples, int count, bool normalize);
            
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
