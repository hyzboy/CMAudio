#include<hgl/audio/AudioResampler.h>
#include<hgl/log/Log.h>
#include<hgl/type/String.h>
#include<hgl/type/StdString.h>
#include<samplerate.h>
#include<cmath>
#include<cstring>
#include<cstdint>

using namespace openal;

namespace hgl::audio
{
    namespace
    {
        struct FormatInfo
        {
            uint bitsPerSample = 0;
            uint channels = 0;
            bool isFloat = false;
        };

        bool ParseFormat(uint format, FormatInfo& info)
        {
            info.bitsPerSample = 0;
            info.channels = 0;
            info.isFloat = false;

            switch(format)
            {
                case AL_FORMAT_MONO8:        info.bitsPerSample=8;  info.channels=1; return true;
                case AL_FORMAT_MONO16:       info.bitsPerSample=16; info.channels=1; return true;
                case AL_FORMAT_MONO_FLOAT32: info.bitsPerSample=32; info.channels=1; info.isFloat=true; return true;

                case AL_FORMAT_STEREO8:        info.bitsPerSample=8;  info.channels=2; return true;
                case AL_FORMAT_STEREO16:       info.bitsPerSample=16; info.channels=2; return true;
                case AL_FORMAT_STEREO_FLOAT32: info.bitsPerSample=32; info.channels=2; info.isFloat=true; return true;

                case AL_FORMAT_QUAD8:  info.bitsPerSample=8;  info.channels=4; return true;
                case AL_FORMAT_QUAD16: info.bitsPerSample=16; info.channels=4; return true;
                case AL_FORMAT_QUAD32: info.bitsPerSample=32; info.channels=4; return true;

                case AL_FORMAT_REAR8:  info.bitsPerSample=8;  info.channels=4; return true;
                case AL_FORMAT_REAR16: info.bitsPerSample=16; info.channels=4; return true;
                case AL_FORMAT_REAR32: info.bitsPerSample=32; info.channels=4; return true;

                case AL_FORMAT_51CHN8:  info.bitsPerSample=8;  info.channels=6; return true;
                case AL_FORMAT_51CHN16: info.bitsPerSample=16; info.channels=6; return true;
                case AL_FORMAT_51CHN32: info.bitsPerSample=32; info.channels=6; return true;

                case AL_FORMAT_61CHN8:  info.bitsPerSample=8;  info.channels=7; return true;
                case AL_FORMAT_61CHN16: info.bitsPerSample=16; info.channels=7; return true;
                case AL_FORMAT_61CHN32: info.bitsPerSample=32; info.channels=7; return true;

                case AL_FORMAT_71CHN8:  info.bitsPerSample=8;  info.channels=8; return true;
                case AL_FORMAT_71CHN16: info.bitsPerSample=16; info.channels=8; return true;
                case AL_FORMAT_71CHN32: info.bitsPerSample=32; info.channels=8; return true;

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

            if(info.bitsPerSample == 32)
            {
                const int32_t* samples = static_cast<const int32_t*>(input);
                for(uint i = 0; i < sampleCount; i++)
                {
                    output[i] = samples[i] / 2147483648.0f;
                }
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

        void ConvertFromFloat(const float* input, uint sampleCount, const FormatInfo& info, void** output, uint* outputSize)
        {
            const uint bytesPerSample = info.bitsPerSample / 8;
            *outputSize = sampleCount * bytesPerSample;
            uint8_t* data = new uint8_t[*outputSize];

            if(info.isFloat && info.bitsPerSample == 32)
            {
                memcpy(data, input, *outputSize);
                *output = data;
                return;
            }

            if(info.bitsPerSample == 32)
            {
                int32_t* dst = reinterpret_cast<int32_t*>(data);
                for(uint i = 0; i < sampleCount; i++)
                {
                    float sample = input[i];
                    if(sample > 1.0f) sample = 1.0f;
                    if(sample < -1.0f) sample = -1.0f;
                    dst[i] = static_cast<int32_t>(sample * 2147483647.0f);
                }
                *output = data;
                return;
            }

            if(info.bitsPerSample == 16)
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

            if(info.bitsPerSample == 8)
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

        if(inputInfo.channels != outputInfo.channels)
        {
            GLogError(OS_TEXT("Input and output channel counts must match for resampling"));
            return false;
        }

        const uint channels = inputInfo.channels;
        const uint bytesPerSample = inputInfo.bitsPerSample / 8;
        const uint inputSampleCount = inputSize / bytesPerSample;
        if(inputSampleCount == 0)
            return false;

        if(inputSampleCount % channels != 0)
        {
            GLogError(OS_TEXT("Input size is not a whole number of frames"));
            return false;
        }

        const uint inputFrameCount = inputSampleCount / channels;

        const double ratio = static_cast<double>(outputSampleRate) / static_cast<double>(inputSampleRate);
        const uint outputFrameCount = static_cast<uint>(std::ceil(inputFrameCount * ratio));
        if(outputFrameCount == 0)
            return false;

        const uint outputSampleCount = outputFrameCount * channels;

        float* inputFloat = new float[inputSampleCount];
        ConvertToFloat(inputData, inputSize, inputFloat, inputSampleCount, inputInfo);

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
        ConvertFromFloat(outputFloat, generated, outputInfo, outputData, outputSize);
        delete[] outputFloat;

        return true;
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
        return Resample(inputData, inputSize, inputFormat, inputSampleRate, outputSampleRate, outputFormat, quality, outputData, outputSize);
    }
}
