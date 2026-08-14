#pragma once

#include<hgl/CoreType.h>
#include<hgl/audio/AudioMixerTypes.h>
#include<hgl/audio/ParametricEQ.h>

namespace hgl::audio
{
    /**
    * 对 PCM 数据原地应用参数化 EQ（P2 实时兜底：EFX 不可用时的 CPU 路径）
    *
    * 支持 int16 / float32 交错数据，逐声道处理（每个声道独立滤波、独立 Reset）。
    * 其它格式（8bit/int24 等）返回 false 且不修改数据。
    *
    * @param data PCM 数据（原地修改）
    * @param size 数据字节数
    * @param info 格式描述（channels/bits_per_sample/is_float/sample_rate）
    * @param eq   参数化均衡器（band 数为 0 时直通返回 true）
    * @return true=已处理（或直通）；false=格式不支持，数据未修改
    */
    bool ApplyEQToPCM(void *data, uint size, const AudioDataInfo &info, ParametricEQ &eq);
}//namespace hgl::audio
