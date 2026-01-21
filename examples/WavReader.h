// Simple WAV Reader Utility
// Reads WAV files into memory
#pragma once

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <hgl/al/al.h>

namespace hgl
{
    namespace audio
    {
        /**
         * Simple WAV file reader
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
             * @param sampleRate Output sample rate
             * @return true if successful
             */
            static bool Load(const char* filename, ALenum* format, void** data, uint32_t* size, uint32_t* sampleRate)
            {
                FILE* file = fopen(filename, "rb");
                if (!file) return false;

                // Get file size
                fseek(file, 0, SEEK_END);
                long fileSize = ftell(file);
                fseek(file, 0, SEEK_SET);

                // Read entire file
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

                // Use the existing WAV parser from the plugin
                ALboolean loop;
                ALsizei alSize, alFreq;
                ALenum alFormat;
                ALvoid* alData;

                // Call the existing alutLoadWAVMemory function
                extern void alutLoadWAVMemory(ALbyte*, ALsizei, ALenum*, ALvoid**, ALsizei*, ALsizei*, ALboolean*);
                alutLoadWAVMemory((ALbyte*)fileData, fileSize, &alFormat, &alData, &alSize, &alFreq, &loop);

                free(fileData);

                if (!alData || alSize == 0)
                    return false;

                *format = alFormat;
                *data = alData;
                *size = (uint32_t)alSize;
                *sampleRate = (uint32_t)alFreq;

                return true;
            }
        };
    } // namespace audio
} // namespace hgl
