// Simple WAV Reader Utility
// Reads WAV files into memory (self-contained RIFF parser)
#pragma once

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <hgl/audio/OpenAL.h>

namespace hgl
{
    namespace audio
    {
        /**
         * Simple WAV file reader (PCM / IEEE float, mono/stereo/multichannel)
         */
        class WavReader
        {
        public:
            /**
             * Load a WAV file into memory
             * @param filename Input filename
             * @param format Output format (AL_FORMAT_*)
             * @param data Output data pointer (caller must free with free())
             * @param size Output data size in bytes
             * @param sample_rate Output sample rate
             * @return true if successful
             */
            static bool Load(const char* filename, openal::ALenum* format, void** data, uint32_t* size, uint32_t* sample_rate)
            {
                if(!filename || !format || !data || !size || !sample_rate)
                    return false;

                FILE* file = fopen(filename, "rb");
                if (!file) return false;

                fseek(file, 0, SEEK_END);
                long fileSize = ftell(file);
                fseek(file, 0, SEEK_SET);

                if(fileSize < 44)   // smallest valid WAV header
                {
                    fclose(file);
                    return false;
                }

                char* fileData = (char*)malloc(fileSize);
                if (!fileData)
                {
                    fclose(file);
                    return false;
                }

                size_t read = fread(fileData, 1, fileSize, file);
                fclose(file);

                if (read != (size_t)fileSize)
                {
                    free(fileData);
                    return false;
                }

                // Parse RIFF/WAVE chunks
                const unsigned char* p = (const unsigned char*)fileData;

                if(memcmp(p, "RIFF", 4) != 0 || memcmp(p+8, "WAVE", 4) != 0)
                {
                    free(fileData);
                    return false;
                }

                p += 12;

                uint16_t channels = 0;
                uint32_t sample_rate_value = 0;
                uint16_t bits_per_sample = 0;
                uint16_t audioFormat = 0;
                const unsigned char* pcmData = nullptr;
                uint32_t pcmSize = 0;

                const unsigned char* end = (const unsigned char*)fileData + fileSize;

                while(p + 8 <= end)
                {
                    char chunkId[5] = {0};
                    memcpy(chunkId, p, 4);

                    uint32_t chunkSize = p[4] | (p[5]<<8) | (p[6]<<16) | (p[7]<<24);

                    if(memcmp(chunkId, "fmt ", 4) == 0 && chunkSize >= 16)
                    {
                        const unsigned char* fmt = p + 8;
                        audioFormat   = fmt[0] | (fmt[1]<<8);          // 1=PCM, 3=IEEE float
                        channels      = fmt[2] | (fmt[3]<<8);
                        sample_rate_value = fmt[4] | (fmt[5]<<8) | (fmt[6]<<16) | (fmt[7]<<24);
                        bits_per_sample = fmt[14] | (fmt[15]<<8);
                    }
                    else if(memcmp(chunkId, "data", 4) == 0)
                    {
                        pcmData = p + 8;
                        pcmSize = chunkSize;
                        break;
                    }

                    p += 8 + chunkSize + (chunkSize & 1);   // chunks are word-aligned
                }

                if(!pcmData || pcmSize == 0 || channels == 0 || sample_rate_value == 0 || bits_per_sample == 0)
                {
                    free(fileData);
                    return false;
                }

                // Build AL format
                openal::ALenum fmt = 0;

                if(audioFormat == 3 && bits_per_sample == 32)     // IEEE float
                {
                    if(channels == 1)       fmt = AL_FORMAT_MONO_FLOAT32;
                    else if(channels == 2)  fmt = AL_FORMAT_STEREO_FLOAT32;
                }
                else if(audioFormat == 1)                       // PCM
                {
                    if(bits_per_sample == 8)
                    {
                        if(channels == 1)       fmt = AL_FORMAT_MONO8;
                        else if(channels == 2)  fmt = AL_FORMAT_STEREO8;
                    }
                    else if(bits_per_sample == 16)
                    {
                        if(channels == 1)       fmt = AL_FORMAT_MONO16;
                        else if(channels == 2)  fmt = AL_FORMAT_STEREO16;
                    }
                }

                if(fmt == 0)
                {
                    free(fileData);
                    return false;
                }

                // Move PCM data to a fresh buffer (caller frees with free())
                void* out = malloc(pcmSize);
                if(!out)
                {
                    free(fileData);
                    return false;
                }

                memcpy(out, pcmData, pcmSize);
                free(fileData);

                *format = fmt;
                *data = out;
                *size = pcmSize;
                *sample_rate = sample_rate_value;

                return true;
            }
        };
    } // namespace audio
} // namespace hgl
