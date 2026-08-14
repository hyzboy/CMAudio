#include<hgl/audio/AudioResampler.h>
#include<hgl/log/Log.h>
#include<hgl/type/String.h>
#include<hgl/type/StdString.h>
#include<samplerate.h>
#include<cmath>
#include<cstring>
#include<cstdint>

namespace hgl::audio
{
    namespace
    {
        void ConvertToFloat(const void* input, uint inputSize, float* output, uint sampleCount, const AudioDataInfo& info)
        {
            if(info.is_float && info.bits_per_sample == 32)
            {
                memcpy(output, input, sampleCount * sizeof(float));
                return;
            }

            if(info.bits_per_sample == 32)
            {
                const int32_t* samples = static_cast<const int32_t*>(input);
                for(uint i = 0; i < sampleCount; i++)
                {
                    output[i] = samples[i] / 2147483648.0f;
                }
                return;
            }

            if(info.bits_per_sample == 16)
            {
                const int16_t* samples = static_cast<const int16_t*>(input);
                for(uint i = 0; i < sampleCount; i++)
                {
                    output[i] = samples[i] / 32768.0f;
                }
                return;
            }

            if(info.bits_per_sample == 8)
            {
                const int8_t* samples = static_cast<const int8_t*>(input);
                for(uint i = 0; i < sampleCount; i++)
                {
                    output[i] = samples[i] / 128.0f;
                }
            }
        }

        void ConvertFromFloat(const float* input, uint sampleCount, const AudioDataInfo& info, void** output, uint* outputSize)
        {
            const uint bytesPerSample = info.bits_per_sample / 8;
            *outputSize = sampleCount * bytesPerSample;
            uint8_t* data = new uint8_t[*outputSize];

            if(info.is_float && info.bits_per_sample == 32)
            {
                memcpy(data, input, *outputSize);
                *output = data;
                return;
            }

            if(info.bits_per_sample == 32)
            {
                int32_t* dst = reinterpret_cast<int32_t*>(data);
                for(uint i = 0; i < sampleCount; i++)
                {
                    float sample = input[i];
                    if(sample > 1.0f) sample = 1.0f;
                    if(sample < -1.0f) sample = -1.0f;
                    dst[i] = static_cast<int32_t>(static_cast<double>(sample) * 2147483647.0);
                }
                *output = data;
                return;
            }

            if(info.bits_per_sample == 16)
            {
                int16_t* dst = reinterpret_cast<int16_t*>(data);
                for(uint i = 0; i < sampleCount; i++)
                {
                    float sample = input[i];
                    if(sample > 1.0f) sample = 1.0f;
                    if(sample < -1.0f) sample = -1.0f;
                    dst[i] = static_cast<int16_t>(sample * 32767.0f);
                }
                *output = data;
                return;
            }

            if(info.bits_per_sample == 8)
            {
                int8_t* dst = reinterpret_cast<int8_t*>(data);
                for(uint i = 0; i < sampleCount; i++)
                {
                    float sample = input[i];
                    if(sample > 1.0f) sample = 1.0f;
                    if(sample < -1.0f) sample = -1.0f;
                    dst[i] = static_cast<int8_t>(sample * 127.0f);
                }
                *output = data;
            }
        }

        int ToLibSampleRateQuality(ResampleQuality quality)
        {
            switch(quality)
            {
                case ResampleQuality::SincFastest:
                    return SRC_SINC_FASTEST;
                case ResampleQuality::SincMedium:
                    return SRC_SINC_MEDIUM_QUALITY;
                case ResampleQuality::SincBest:
                    return SRC_SINC_BEST_QUALITY;
                case ResampleQuality::Linear:
                default:
                    return SRC_LINEAR;
            }
        }
    }

    bool Resample(const void* inputData,
                  uint inputSize,
                  const AudioDataInfo &inputInfo,
                  uint outputSampleRate,
                  const AudioDataInfo &outputInfo,
                  ResampleQuality quality,
                  void** outputData,
                  uint* outputSize)
    {
        if(!inputData || inputSize == 0 || inputInfo.sample_rate == 0 || outputSampleRate == 0)
            return false;

        if(!outputData || !outputSize)
            return false;

        const AudioDataInfo &inputInfoRef = inputInfo;

        AudioDataInfo outInfo = outputInfo;
        if(outInfo.channels == 0 || outInfo.bits_per_sample == 0)
        {
            outInfo.channels = inputInfoRef.channels;
            outInfo.bits_per_sample = inputInfoRef.bits_per_sample;
            outInfo.is_float = inputInfoRef.is_float;
        }

        if(inputInfoRef.channels != outInfo.channels)
        {
            GLogError(OS_TEXT("Input and output channel counts must match for resampling"));
            return false;
        }

        const uint channels = inputInfoRef.channels;
        const uint bytesPerSample = inputInfoRef.bits_per_sample / 8;
        const uint inputSampleCount = inputSize / bytesPerSample;
        if(inputSampleCount == 0)
            return false;

        if(inputSampleCount % channels != 0)
        {
            GLogError(OS_TEXT("Input size is not a whole number of frames"));
            return false;
        }

        const uint inputFrameCount = inputSampleCount / channels;

        const double ratio = static_cast<double>(outputSampleRate) / static_cast<double>(inputInfoRef.sample_rate);
        const uint outputFrameCount = static_cast<uint>(std::ceil(inputFrameCount * ratio));
        if(outputFrameCount == 0)
            return false;

        const uint outputSampleCount = outputFrameCount * channels;

        float* inputFloat = new float[inputSampleCount];
        ConvertToFloat(inputData, inputSize, inputFloat, inputSampleCount, inputInfoRef);

        float* outputFloat = new float[outputSampleCount];

        SRC_DATA data;
        data.data_in = inputFloat;
        data.data_out = outputFloat;
        data.input_frames = static_cast<long>(inputFrameCount);
        data.output_frames = static_cast<long>(outputFrameCount);
        data.end_of_input = 1;
        data.src_ratio = ratio;

        const int converter = ToLibSampleRateQuality(quality);
        const int result = src_simple(&data, converter, static_cast<int>(channels));
        if(result != 0)
        {
            GLogError(OS_TEXT("libsamplerate failed"));
            GLogError(ToOSString(std::string(src_strerror(result))));
            delete[] inputFloat;
            delete[] outputFloat;
            return false;
        }

        const uint generatedFrames = static_cast<uint>(data.output_frames_gen);
        if(generatedFrames == 0)
        {
            delete[] inputFloat;
            delete[] outputFloat;
            return false;
        }

        delete[] inputFloat;

        const uint generated = generatedFrames * channels;
        ConvertFromFloat(outputFloat, generated, outInfo, outputData, outputSize);
        delete[] outputFloat;

        return true;
    }

    bool ResampleMono(const void* inputData,
                      uint inputSize,
                      const AudioDataInfo &inputInfo,
                      uint outputSampleRate,
                      const AudioDataInfo &outputInfo,
                      ResampleQuality quality,
                      void** outputData,
                      uint* outputSize)
    {
        return Resample(inputData, inputSize, inputInfo, outputSampleRate, outputInfo, quality, outputData, outputSize);
    }
}
