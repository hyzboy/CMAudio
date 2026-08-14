#pragma once

#include<hgl/CoreType.h>
#include<hgl/audio/AudioMixerTypes.h>

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
     * - If outputInfo.sampleRate is 0, the input sample rate is preserved;
     *   if outputInfo.channels is 0, the input format is preserved.
     * - Caller owns outputData and must delete[] it.
     */
    bool Resample(const void* inputData,
                  uint inputSize,
                  const AudioDataInfo &inputInfo,
                  uint outputSampleRate,
                  const AudioDataInfo &outputInfo,
                  ResampleQuality quality,
                  void** outputData,
                  uint* outputSize);

    /**
     * Backward-compatible alias of Resample(), kept for existing mono callers.
     */
    bool ResampleMono(const void* inputData,
                      uint inputSize,
                      const AudioDataInfo &inputInfo,
                      uint outputSampleRate,
                      const AudioDataInfo &outputInfo,
                      ResampleQuality quality,
                      void** outputData,
                      uint* outputSize);
}
