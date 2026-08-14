#pragma once

#include<hgl/audio/AudioMixerTypes.h>
#include<hgl/audio/AudioMemoryPool.h>
#include<hgl/audio/OpenAL.h>
#include<hgl/audio/ParametricEQ.h>
#include<hgl/type/ValueArray.h>
#include<hgl/log/Log.h>

namespace hgl::audio
{
    /**
     * 音频混音器
     * 用于将多个音轨叠加混音成一个新的音频数据
     * 支持单声道及多声道音频，支持时间偏移、音量调整、音调变化等变换
     * 多个音源必须统一格式与采样率，不在此处进行重采样
     */
    class AudioMixer
    {
        OBJECT_LOGGER

    private:

        // Pitch shifting constants
        static constexpr float MinPitch = 0.5f;
        static constexpr float MaxPitch = 2.0f;
        static constexpr float DefaultPitch = 1.0f;

        struct SourceAudio
        {
            AudioDataInfo info;
            const void* data;
            uint data_size;

            bool operator==(const SourceAudio& other) const
            {
                return data == other.data &&
                       data_size == other.data_size &&
                       info.sample_rate == other.info.sample_rate &&
                       info.channels == other.info.channels &&
                       info.bits_per_sample == other.info.bits_per_sample &&
                       info.is_float == other.info.is_float;
            }
        };

        ValueArray<SourceAudio> sources;        ///< 音源列表
        ValueArray<MixingTrack> tracks;         ///< 混音轨道列表
        MixerConfig config;                     ///< 混音器配置

        AudioDataInfo common_info;               ///< 统一音频格式信息
        bool has_common_info;                     ///< 是否已设置统一格式
        AudioDataInfo output_format;             ///< 输出格式

        ParametricEQ eq;                        ///< 参数化均衡器（P2：混音输出前应用）

        // 内存池 - 避免频繁分配/释放
        AudioMemoryPool<float> pool_buffer;      ///< 主混音缓冲池
        AudioMemoryPool<float> temp_buffer;      ///< 临时格式转换缓冲池

        /**
            * 将整数采样转换为浮点 (-1.0 到 1.0)
            */
        void ConvertToFloat(const void* input, uint inputSize, float** output, uint* outputCount, const AudioDataInfo& info);

        /**
            * 将浮点采样转换为目标格式
            */
        void ConvertFromFloat(const float* input, uint sampleCount, void** output, uint* outputSize, const AudioDataInfo& outputInfo);

        /**
            * 应用音调变化(libsamplerate 高质量重采样) - float版本
            * 按帧处理，pitch>1 升调(变快/变短)，pitch<1 降调(变慢/变长)
            */
        void ApplyPitchShift(const float* input, uint inputFrameCount, uint channels,
                            float** output, uint* outputFrameCount, float pitch);

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
            * 使用 Mersenne Twister 高质量随机源（替代低质量 C rand()）
            * @return 范围在[-1.0, 1.0]的随机噪声值
            */
        static float GenerateTPDFDither();

    public:

        AudioMixer();
        virtual ~AudioMixer();

        /**
         * 添加音源数据
         * @param info 音频数据信息（声道数、位深、是否浮点、采样率、数据大小）
         * @param data 音频数据指针
         * @return 音源索引，失败返回 -1
         */
        int AddSourceAudio(const AudioDataInfo &info,const void *data);

        /**
         * 添加音源数据(便捷方法)
         * @param data 音频数据指针
         * @param size 数据大小
         * @param format 音频格式 (AL_FORMAT_*)
         * @param sample_rate 采样率
         * @return 音源索引，失败返回 -1
         */
        int AddSourceAudio(const void *data,uint size,uint format,uint sample_rate);

        /**
            * 清除所有音源
            */
        void ClearSources();

        /**
            * 获取音源数量
            */
        int GetSourceCount() const { return sources.GetCount(); }

        /**
            * 添加混音轨道
            * @param track 轨道参数
            */
        void AddTrack(const MixingTrack& track);

        /**
            * 添加混音轨道(便捷方法)
            * @param source_index 音源索引
            * @param time_offset 时间偏移(秒)
            * @param volume 音量(0.0-1.0)
            * @param pitch 音调(0.5-2.0)
            */
        void AddTrack(uint source_index, float time_offset, float volume, float pitch);

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
         * 设置输出格式
         * @param info 输出格式信息（声道数、位深、是否浮点）
         * @return 是否成功
         */
        bool SetOutputFormat(const AudioDataInfo &info) { output_format=info; return true; }

        /**
         * 设置输出格式(便捷方法)
         * @param format 输出格式 (AL_FORMAT_*，须与音源声道数一致)
         * @return 是否成功
         */
        bool SetOutputFormat(uint format)
        {
            AudioDataInfo info;
            if(!openal::FromOpenALFormat(format,info))
                return false;
            output_format=info;
            return true;
        }

        /**
         * 获取输出格式
         */
        const AudioDataInfo &GetOutputFormat() const { return output_format; }

        /**
         * 取得参数化均衡器（可直接 AddBand/SetBand 配置频段，Mix() 输出前应用）
         */
        ParametricEQ &GetEQ() { return eq; }
        const ParametricEQ &GetEQ() const { return eq; }

        /**
         * 清空 EQ 频段（等效直通）
         */
        void ClearEQ() { eq.ClearBands(); }

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
        const AudioDataInfo& GetOutputInfo() const { return common_info; }
    };
}//namespace hgl::audio
