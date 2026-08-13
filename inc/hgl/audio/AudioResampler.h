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
     * Resample raw interleaved audio data to a new sample rate and/or format.
     * - Uses libsamplerate for resampling.
     * - Supports mono, stereo, quad, 5.1, 6.1 and 7.1 layouts (8/16/32-bit
     *   integer and float32 samples).
     * - Input and output channel counts must match (resampling does not
     *   remap channel layouts).
     * - If outputFormat is 0, the input format is preserved.
     * - Caller owns outputData and must delete[] it.
     */
    bool Resample(const void* inputData,
                  uint inputSize,
                  uint inputFormat,
                  uint inputSampleRate,
                  uint outputSampleRate,
                  uint outputFormat,
                  ResampleQuality quality,
                  void** outputData,
                  uint* outputSize);

    /**
     * Backward-compatible alias of Resample(), kept for existing mono callers.
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
