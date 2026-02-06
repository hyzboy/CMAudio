#include<hgl/audio/AudioResampler.h>
#include<hgl/log/Log.h>
#include<hgl/type/String.h>
#include<hgl/type/StdString.h>
#include<samplerate.h>
#include<cmath>
#include<cstring>

using namespace openal;

namespace hgl::audio
{
    namespace
    {
        struct FormatInfo
        {
            uint bitsPerSample = 0;
            bool isFloat = false;
        };

        bool ParseFormat(uint format, FormatInfo& info)
        {
            info.bitsPerSample = 0;
            info.isFloat = false;

            switch(format)
            {
                case AL_FORMAT_MONO8:
                    info.bitsPerSample = 8;
                    return true;
                case AL_FORMAT_MONO16:
                    info.bitsPerSample = 16;
                    return true;
                case AL_FORMAT_MONO_FLOAT32:
                    info.bitsPerSample = 32;
                    info.isFloat = true;
                    return true;
                default:
                    return false;
            }
        }

        void ConvertToFloat(const void* input, uint inputSize, float* output, uint sampleCount, const FormatInfo& info)
        {
            if(info.isFloat && info.bitsPerSample == 32)
            {
                memcpy(output, input, sampleCount * sizeof(float));
                return;
            }

            if(info.bitsPerSample == 16)
            {
                const int16_t* samples = static_cast<const int16_t*>(input);
                for(uint i = 0; i < sampleCount; i++)
                {
                    output[i] = samples[i] / 32768.0f;
                }
                return;
            }

            if(info.bitsPerSample == 8)
            {
                const int8_t* samples = static_cast<const int8_t*>(input);
                for(uint i = 0; i < sampleCount; i++)
                {
                    output[i] = samples[i] / 128.0f;
                }
            }
        }

        void ConvertFromFloat(const float* input, uint sampleCount, void** output, uint* outputSize, uint targetFormat)
        {
            if(targetFormat == AL_FORMAT_MONO_FLOAT32)
            {
                *outputSize = sampleCount * sizeof(float);
                float* data = new float[sampleCount];
                memcpy(data, input, *outputSize);
                *output = data;
                return;
            }

            if(targetFormat == AL_FORMAT_MONO16)
            {
                *outputSize = sampleCount * sizeof(int16_t);
                int16_t* data = new int16_t[sampleCount];
                for(uint i = 0; i < sampleCount; i++)
                {
                    float sample = input[i];
                    if(sample > 1.0f) sample = 1.0f;
                    if(sample < -1.0f) sample = -1.0f;
                    data[i] = static_cast<int16_t>(sample * 32767.0f);
                }
                *output = data;
                return;
            }

            if(targetFormat == AL_FORMAT_MONO8)
            {
                *outputSize = sampleCount * sizeof(int8_t);
                int8_t* data = new int8_t[sampleCount];
                for(uint i = 0; i < sampleCount; i++)
                {
                    float sample = input[i];
                    if(sample > 1.0f) sample = 1.0f;
                    if(sample < -1.0f) sample = -1.0f;
                    data[i] = static_cast<int8_t>(sample * 127.0f);
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

    bool ResampleMono(const void* inputData,
                      uint inputSize,
                      uint inputFormat,
                      uint inputSampleRate,
                      uint outputSampleRate,
                      uint outputFormat,
                      ResampleQuality quality,
                      void** outputData,
                      uint* outputSize)
    {
        if(!inputData || inputSize == 0 || inputSampleRate == 0 || outputSampleRate == 0)
            return false;

        if(!outputData || !outputSize)
            return false;

        FormatInfo inputInfo;
        if(!ParseFormat(inputFormat, inputInfo))
        {
            GLogError(OS_TEXT("Unsupported input format for resampler"));
            return false;
        }

        if(outputFormat == 0)
            outputFormat = inputFormat;

        FormatInfo outputInfo;
        if(!ParseFormat(outputFormat, outputInfo))
        {
            GLogError(OS_TEXT("Unsupported output format for resampler"));
            return false;
        }

        uint bytesPerSample = inputInfo.bitsPerSample / 8;
        uint inputSampleCount = inputSize / bytesPerSample;
        if(inputSampleCount == 0)
            return false;

        double ratio = static_cast<double>(outputSampleRate) / static_cast<double>(inputSampleRate);
        uint outputSampleCount = static_cast<uint>(std::ceil(inputSampleCount * ratio));
        if(outputSampleCount == 0)
            return false;

        float* inputFloat = new float[inputSampleCount];
        ConvertToFloat(inputData, inputSize, inputFloat, inputSampleCount, inputInfo);

        float* outputFloat = new float[outputSampleCount];

        SRC_DATA data;
        data.data_in = inputFloat;
        data.data_out = outputFloat;
        data.input_frames = static_cast<long>(inputSampleCount);
        data.output_frames = static_cast<long>(outputSampleCount);
        data.end_of_input = 1;
        data.src_ratio = ratio;

        int converter = ToLibSampleRateQuality(quality);
        int result = src_simple(&data, converter, 1);
        if(result != 0)
        {
            GLogError(OS_TEXT("libsamplerate failed"));
            GLogError(ToOSString(std::string(src_strerror(result))));
            delete[] inputFloat;
            delete[] outputFloat;
            return false;
        }

        uint generated = static_cast<uint>(data.output_frames_gen);
        if(generated == 0)
        {
            delete[] inputFloat;
            delete[] outputFloat;
            return false;
        }

        delete[] inputFloat;

        ConvertFromFloat(outputFloat, generated, outputData, outputSize, outputFormat);
        delete[] outputFloat;

        return true;
    }
}
