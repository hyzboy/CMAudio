#pragma once

#include<hgl/type/BaseString.h>
#include<hgl/type/List.h>

namespace hgl
{
    namespace audio
    {
        /**
         * 混音轨道参数
         * 用于定义单个音轨在混音时的变换参数
         */
        struct MixingTrack
        {
            float timeOffset;       ///< 时间偏移(秒) - 音轨开始播放的时间偏移
            float volume;           ///< 音量(0.0-1.0) - 音轨的音量比例
            float pitch;            ///< 音调(0.5-2.0) - 音调变化，1.0为原始音调
            
            MixingTrack()
            {
                timeOffset = 0.0f;
                volume = 1.0f;
                pitch = 1.0f;
            }
            
            MixingTrack(float time_offset, float vol, float p)
            {
                timeOffset = time_offset;
                volume = vol;
                pitch = p;
            }
        };
        
        /**
         * 混音器配置
         */
        struct MixerConfig
        {
            bool normalize;         ///< 是否归一化输出，防止溢出
            float masterVolume;     ///< 主音量(0.0-1.0)
            
            MixerConfig()
            {
                normalize = true;
                masterVolume = 1.0f;
            }
        };
        
        /**
         * 音频数据信息
         */
        struct AudioDataInfo
        {
            uint format;            ///< 音频格式 (AL_FORMAT_*)
            uint sampleRate;        ///< 采样率
            uint channels;          ///< 声道数 (1=mono, 2=stereo)
            uint bitsPerSample;     ///< 每个采样的位数 (8, 16)
            uint dataSize;          ///< 数据大小(字节)
            
            AudioDataInfo()
            {
                format = 0;
                sampleRate = 0;
                channels = 0;
                bitsPerSample = 0;
                dataSize = 0;
            }
        };
        
    }//namespace audio
}//namespace hgl
