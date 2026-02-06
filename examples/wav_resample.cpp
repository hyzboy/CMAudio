// WAV Resampler Example
// Resamples mono or stereo WAV files
#include <iostream>
#include <vector>
#include <cstring>
#include <hgl/audio/AudioResampler.h>
#include "WavReader.h"
#include "WavWriter.h"

using namespace hgl::audio;

namespace
{
    bool GetFormatInfo(ALenum format, uint16_t& channels, uint16_t& bitsPerSample, ALenum& monoFormat)
    {
        switch(format)
        {
            case AL_FORMAT_MONO8:
                channels = 1;
                bitsPerSample = 8;
                monoFormat = AL_FORMAT_MONO8;
                return true;
            case AL_FORMAT_MONO16:
                channels = 1;
                bitsPerSample = 16;
                monoFormat = AL_FORMAT_MONO16;
                return true;
            case AL_FORMAT_MONO_FLOAT32:
                channels = 1;
                bitsPerSample = 32;
                monoFormat = AL_FORMAT_MONO_FLOAT32;
                return true;
            case AL_FORMAT_STEREO8:
                channels = 2;
                bitsPerSample = 8;
                monoFormat = AL_FORMAT_MONO8;
                return true;
            case AL_FORMAT_STEREO16:
                channels = 2;
                bitsPerSample = 16;
                monoFormat = AL_FORMAT_MONO16;
                return true;
#ifdef AL_FORMAT_STEREO_FLOAT32
            case AL_FORMAT_STEREO_FLOAT32:
                channels = 2;
                bitsPerSample = 32;
                monoFormat = AL_FORMAT_MONO_FLOAT32;
                return true;
#endif
            default:
                return false;
        }
    }

    ALenum MakeStereoFormat(ALenum monoFormat)
    {
        switch(monoFormat)
        {
            case AL_FORMAT_MONO8: return AL_FORMAT_STEREO8;
            case AL_FORMAT_MONO16: return AL_FORMAT_STEREO16;
#ifdef AL_FORMAT_STEREO_FLOAT32
            case AL_FORMAT_MONO_FLOAT32: return AL_FORMAT_STEREO_FLOAT32;
#endif
            default: return 0;
        }
    }

    void SplitStereo(const void* interleaved, uint32_t frameCount, uint16_t bitsPerSample,
                     std::vector<uint8_t>& left, std::vector<uint8_t>& right)
    {
        uint32_t bytesPerSample = bitsPerSample / 8;
        left.resize(frameCount * bytesPerSample);
        right.resize(frameCount * bytesPerSample);

        if(bitsPerSample == 16)
        {
            const int16_t* src = static_cast<const int16_t*>(interleaved);
            int16_t* leftDst = reinterpret_cast<int16_t*>(left.data());
            int16_t* rightDst = reinterpret_cast<int16_t*>(right.data());
            for(uint32_t i = 0; i < frameCount; i++)
            {
                leftDst[i] = src[i * 2];
                rightDst[i] = src[i * 2 + 1];
            }
        }
        else if(bitsPerSample == 8)
        {
            const int8_t* src = static_cast<const int8_t*>(interleaved);
            int8_t* leftDst = reinterpret_cast<int8_t*>(left.data());
            int8_t* rightDst = reinterpret_cast<int8_t*>(right.data());
            for(uint32_t i = 0; i < frameCount; i++)
            {
                leftDst[i] = src[i * 2];
                rightDst[i] = src[i * 2 + 1];
            }
        }
        else if(bitsPerSample == 32)
        {
            const float* src = static_cast<const float*>(interleaved);
            float* leftDst = reinterpret_cast<float*>(left.data());
            float* rightDst = reinterpret_cast<float*>(right.data());
            for(uint32_t i = 0; i < frameCount; i++)
            {
                leftDst[i] = src[i * 2];
                rightDst[i] = src[i * 2 + 1];
            }
        }
    }

    void InterleaveStereo(const void* leftData, const void* rightData, uint32_t frameCount, uint16_t bitsPerSample,
                          std::vector<uint8_t>& output)
    {
        uint32_t bytesPerSample = bitsPerSample / 8;
        output.resize(frameCount * bytesPerSample * 2);

        if(bitsPerSample == 16)
        {
            const int16_t* leftSrc = static_cast<const int16_t*>(leftData);
            const int16_t* rightSrc = static_cast<const int16_t*>(rightData);
            int16_t* dst = reinterpret_cast<int16_t*>(output.data());
            for(uint32_t i = 0; i < frameCount; i++)
            {
                dst[i * 2] = leftSrc[i];
                dst[i * 2 + 1] = rightSrc[i];
            }
        }
        else if(bitsPerSample == 8)
        {
            const int8_t* leftSrc = static_cast<const int8_t*>(leftData);
            const int8_t* rightSrc = static_cast<const int8_t*>(rightData);
            int8_t* dst = reinterpret_cast<int8_t*>(output.data());
            for(uint32_t i = 0; i < frameCount; i++)
            {
                dst[i * 2] = leftSrc[i];
                dst[i * 2 + 1] = rightSrc[i];
            }
        }
        else if(bitsPerSample == 32)
        {
            const float* leftSrc = static_cast<const float*>(leftData);
            const float* rightSrc = static_cast<const float*>(rightData);
            float* dst = reinterpret_cast<float*>(output.data());
            for(uint32_t i = 0; i < frameCount; i++)
            {
                dst[i * 2] = leftSrc[i];
                dst[i * 2 + 1] = rightSrc[i];
            }
        }
    }
}

int main(int argc, char** argv)
{
    if(argc < 4)
    {
        std::cout << "Usage: wav_resample <input.wav> <output.wav> <target_sample_rate> [linear|sinc]\n";
        return 1;
    }

    const char* inputFile = argv[1];
    const char* outputFile = argv[2];
    uint32_t targetRate = static_cast<uint32_t>(std::atoi(argv[3]));
    ResampleQuality quality = ResampleQuality::Linear;

    if(argc >= 5)
    {
        if(std::strcmp(argv[4], "sinc") == 0)
            quality = ResampleQuality::Sinc;
    }

    ALenum format = 0;
    void* data = nullptr;
    uint32_t size = 0;
    uint32_t sampleRate = 0;

    if(!WavReader::Load(inputFile, &format, &data, &size, &sampleRate))
    {
        std::cerr << "Failed to load WAV: " << inputFile << "\n";
        return 1;
    }

    uint16_t channels = 0;
    uint16_t bitsPerSample = 0;
    ALenum monoFormat = 0;
    if(!GetFormatInfo(format, channels, bitsPerSample, monoFormat))
    {
        std::cerr << "Unsupported format in input WAV\n";
        free(data);
        return 1;
    }

    if(targetRate == 0)
    {
        std::cerr << "Invalid target sample rate\n";
        free(data);
        return 1;
    }

    void* outData = nullptr;
    uint32_t outSize = 0;
    ALenum outputFormat = format;

    if(channels == 1)
    {
        if(!ResampleMono(data, size, monoFormat, sampleRate, targetRate, monoFormat, quality, &outData, &outSize))
        {
            std::cerr << "Resample failed\n";
            free(data);
            return 1;
        }
    }
    else
    {
        uint32_t bytesPerSample = bitsPerSample / 8;
        uint32_t frameCount = size / (bytesPerSample * 2);

        std::vector<uint8_t> left;
        std::vector<uint8_t> right;
        SplitStereo(data, frameCount, bitsPerSample, left, right);

        void* leftOut = nullptr;
        void* rightOut = nullptr;
        uint32_t leftOutSize = 0;
        uint32_t rightOutSize = 0;

        if(!ResampleMono(left.data(), (uint)left.size(), monoFormat, sampleRate, targetRate, monoFormat, quality, &leftOut, &leftOutSize))
        {
            std::cerr << "Left channel resample failed\n";
            free(data);
            return 1;
        }

        if(!ResampleMono(right.data(), (uint)right.size(), monoFormat, sampleRate, targetRate, monoFormat, quality, &rightOut, &rightOutSize))
        {
            std::cerr << "Right channel resample failed\n";
            delete[] (char*)leftOut;
            free(data);
            return 1;
        }

        uint32_t leftFrames = leftOutSize / bytesPerSample;
        uint32_t rightFrames = rightOutSize / bytesPerSample;
        uint32_t outFrames = (leftFrames < rightFrames) ? leftFrames : rightFrames;

        std::vector<uint8_t> interleaved;
        InterleaveStereo(leftOut, rightOut, outFrames, bitsPerSample, interleaved);

        outSize = (uint32_t)interleaved.size();
        outData = new uint8_t[outSize];
        memcpy(outData, interleaved.data(), outSize);

        delete[] (char*)leftOut;
        delete[] (char*)rightOut;
    }

    WavWriter writer;
    if(!writer.Open(outputFile, outputFormat, targetRate))
    {
        std::cerr << "Failed to open output WAV\n";
        delete[] (char*)outData;
        free(data);
        return 1;
    }

    if(!writer.Write(outData, outSize))
    {
        std::cerr << "Failed to write WAV data\n";
        delete[] (char*)outData;
        free(data);
        return 1;
    }

    writer.Close();

    delete[] (char*)outData;
    free(data);

    std::cout << "Resampled to " << targetRate << " Hz: " << outputFile << "\n";
    return 0;
}
