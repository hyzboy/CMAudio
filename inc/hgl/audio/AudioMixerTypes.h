#pragma once

#include<hgl/CoreType.h>

namespace hgl::audio
{
        /**
         * 混音轨道参数
         * 用于定义单个音轨在混音时的变换参数
         */
        struct MixingTrack
        {
            uint sourceIndex;      ///< 音源索引 - 指向混音器内部的音源列表
            float timeOffset;       ///< 时间偏移(秒) - 音轨开始播放的时间偏移
            float volume;           ///< 音量(0.0-1.0) - 音轨的音量比例
            float pitch;            ///< 音调(0.5-2.0) - 音调变化，1.0为原始音调

            MixingTrack()
            {
                sourceIndex = 0;
                timeOffset = 0.0f;
                volume = 1.0f;
                pitch = 1.0f;
            }

            MixingTrack(uint source_index, float time_offset, float vol, float p)
            {
                sourceIndex = source_index;
                timeOffset = time_offset;
                volume = vol;
                pitch = p;
            }

            bool operator==(const MixingTrack& other) const
            {
                return sourceIndex == other.sourceIndex &&
                       timeOffset == other.timeOffset && 
                       volume == other.volume && 
                       pitch == other.pitch;
            }
        };

        /**
         * 混音器配置
         */
        struct MixerConfig
        {
            bool normalize;         ///< 是否归一化输出，防止溢出
            float masterVolume;     ///< 主音量(0.0-1.0)
            bool useSoftClipper;    ///< 是否使用软削波器（Soft Clipper）处理越界的float32数据
            bool useDither;         ///< 是否在float32→int16转换时使用抖动（Dither）减少量化噪声

            MixerConfig()
            {
                normalize = true;
                masterVolume = 1.0f;
                useSoftClipper = false;  // 默认关闭，使用硬削波
                useDither = false;       // 默认关闭抖动
            }
        };

        /**
         * 音频数据信息
         * 声道数+位深+是否浮点 三元组完全描述采样格式，不依赖任何后端格式枚举
         */
        struct AudioDataInfo
        {
            uint sampleRate;        ///< 采样率
            uint channels;          ///< 声道数 (1/2/4/6/7/8)
            uint bitsPerSample;     ///< 每个采样的位数 (8, 16, 32)
            bool isFloat;           ///< 是否为浮点格式
            uint dataSize;          ///< 数据大小(字节)

            AudioDataInfo()
            {
                sampleRate = 0;
                channels = 0;
                bitsPerSample = 0;
                isFloat = false;
                dataSize = 0;
            }
        };
        }//namespace hgl::audio
