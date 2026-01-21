// Simple WAV Writer Utility
// Writes mono PCM audio data to WAV files
#pragma once

#include <cstdio>
#include <cstdint>
#include <hgl/al/al.h>

namespace hgl
{
    namespace audio
    {
        /**
         * Simple WAV file writer for mono audio
         * Supports AL_FORMAT_MONO8 and AL_FORMAT_MONO16
         */
        class WavWriter
        {
        private:
            FILE* file;
            uint32_t dataSize;
            long dataSizePos;

            struct WavHeader
            {
                // RIFF chunk
                char riffID[4];          // "RIFF"
                uint32_t fileSize;       // File size - 8
                char waveID[4];          // "WAVE"
                
                // fmt chunk
                char fmtID[4];           // "fmt "
                uint32_t fmtSize;        // Size of fmt chunk (16 for PCM)
                uint16_t audioFormat;    // 1 for PCM
                uint16_t numChannels;    // 1 for mono, 2 for stereo
                uint32_t sampleRate;     // Sample rate
                uint32_t byteRate;       // SampleRate * NumChannels * BitsPerSample/8
                uint16_t blockAlign;     // NumChannels * BitsPerSample/8
                uint16_t bitsPerSample;  // 8 or 16
                
                // data chunk
                char dataID[4];          // "data"
                uint32_t dataSize;       // Size of data
            };

        public:
            WavWriter() : file(nullptr), dataSize(0), dataSizePos(0) {}
            
            ~WavWriter()
            {
                Close();
            }

            /**
             * Open a WAV file for writing
             * @param filename Output filename
             * @param format AL_FORMAT_MONO8 or AL_FORMAT_MONO16
             * @param sampleRate Sample rate in Hz
             * @return true if successful
             */
            bool Open(const char* filename, ALenum format, uint32_t sampleRate)
            {
                if (file) Close();

                file = fopen(filename, "wb");
                if (!file) return false;

                // Determine bits per sample from format
                uint16_t bitsPerSample = 0;
                if (format == AL_FORMAT_MONO8)
                    bitsPerSample = 8;
                else if (format == AL_FORMAT_MONO16)
                    bitsPerSample = 16;
                else
                {
                    fclose(file);
                    file = nullptr;
                    return false;
                }

                // Write header
                WavHeader header;
                memcpy(header.riffID, "RIFF", 4);
                header.fileSize = 0;  // Will update on close
                memcpy(header.waveID, "WAVE", 4);
                memcpy(header.fmtID, "fmt ", 4);
                header.fmtSize = 16;
                header.audioFormat = 1;  // PCM
                header.numChannels = 1;  // Mono
                header.sampleRate = sampleRate;
                header.bitsPerSample = bitsPerSample;
                header.byteRate = sampleRate * 1 * bitsPerSample / 8;
                header.blockAlign = 1 * bitsPerSample / 8;
                memcpy(header.dataID, "data", 4);
                header.dataSize = 0;  // Will update on close

                fwrite(&header, sizeof(WavHeader), 1, file);
                
                // Remember position of data size field
                dataSizePos = sizeof(WavHeader) - sizeof(uint32_t);
                dataSize = 0;

                return true;
            }

            /**
             * Write audio data to the file
             * @param data Pointer to audio data
             * @param size Size of data in bytes
             * @return true if successful
             */
            bool Write(const void* data, uint32_t size)
            {
                if (!file) return false;
                
                size_t written = fwrite(data, 1, size, file);
                if (written != size) return false;
                
                dataSize += size;
                return true;
            }

            /**
             * Close the file and update header with correct sizes
             */
            void Close()
            {
                if (!file) return;

                // Update data size
                fseek(file, dataSizePos, SEEK_SET);
                fwrite(&dataSize, sizeof(uint32_t), 1, file);

                // Update file size
                uint32_t fileSize = dataSize + sizeof(WavHeader) - 8;
                fseek(file, 4, SEEK_SET);
                fwrite(&fileSize, sizeof(uint32_t), 1, file);

                fclose(file);
                file = nullptr;
                dataSize = 0;
            }
        };
    } // namespace audio
} // namespace hgl
