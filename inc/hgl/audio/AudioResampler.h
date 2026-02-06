#pragma once

#include<hgl/CoreType.h>
#include<hgl/audio/OpenAL.h>

namespace hgl::audio
{
    enum class ResampleQuality
    {
        Linear,
        SincFastest,
        SincMedium,
        SincBest
    };

    /**
     * Resample raw mono audio data to a new sample rate and/or format.
     * - Uses libsamplerate for resampling.
     * - Only mono formats are supported.
     * - If outputFormat is 0, the input format is preserved.
     * - Caller owns outputData and must delete[] it.
     */
    bool ResampleMono(const void* inputData,
                      uint inputSize,
                      uint inputFormat,
                      uint inputSampleRate,
                      uint outputSampleRate,
                      uint outputFormat,
                      ResampleQuality quality,
                      void** outputData,
                      uint* outputSize);
}
