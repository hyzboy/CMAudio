// Simple WAV Writer Utility
// Writes mono PCM audio data to WAV files
#pragma once

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <hgl/audio/OpenAL.h>

namespace hgl
{
    namespace audio
    {
        /**
         * Simple WAV file writer for mono/stereo audio
         * Supports AL_FORMAT_MONO8/MONO16/MONO_FLOAT32 and AL_FORMAT_STEREO8/STEREO16/STEREO_FLOAT32
         */
        class WavWriter
        {
        private:
            FILE* file;
            uint32_t data_size;
            long data_size_pos;

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
                uint32_t sample_rate;     // Sample rate
                uint32_t byteRate;       // SampleRate * NumChannels * BitsPerSample/8
                uint16_t blockAlign;     // NumChannels * BitsPerSample/8
                uint16_t bits_per_sample;  // 8 or 16

                // data chunk
                char dataID[4];          // "data"
                uint32_t data_size;       // Size of data
            };

        public:
            WavWriter() : file(nullptr), data_size(0), data_size_pos(0) {}

            ~WavWriter()
            {
                Close();
            }

            /**
             * Open a WAV file for writing
             * @param filename Output filename
             * @param format AL_FORMAT_MONO8, AL_FORMAT_MONO16, or AL_FORMAT_MONO_FLOAT32
             * @param sample_rate Sample rate in Hz
             * @return true if successful
             */
            bool Open(const char* filename, openal::ALenum format, uint32_t sample_rate)
            {
                if (file) Close();

                file = fopen(filename, "wb");
                if (!file) return false;

                // Determine bits per sample and audio format from format
                uint16_t bits_per_sample = 0;
                uint16_t audioFormat = 1;  // 1 = PCM, 3 = IEEE float
                uint16_t numChannels = 0;

                if (format == AL_FORMAT_MONO8)
                {
                    bits_per_sample = 8;
                    audioFormat = 1;
                    numChannels = 1;
                }
                else if (format == AL_FORMAT_MONO16)
                {
                    bits_per_sample = 16;
                    audioFormat = 1;
                    numChannels = 1;
                }
                else if (format == AL_FORMAT_MONO_FLOAT32)
                {
                    bits_per_sample = 32;
                    audioFormat = 3;  // IEEE float
                    numChannels = 1;
                }
                else if (format == AL_FORMAT_STEREO8)
                {
                    bits_per_sample = 8;
                    audioFormat = 1;
                    numChannels = 2;
                }
                else if (format == AL_FORMAT_STEREO16)
                {
                    bits_per_sample = 16;
                    audioFormat = 1;
                    numChannels = 2;
                }
#ifdef AL_FORMAT_STEREO_FLOAT32
                else if (format == AL_FORMAT_STEREO_FLOAT32)
                {
                    bits_per_sample = 32;
                    audioFormat = 3;
                    numChannels = 2;
                }
#endif
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
                header.audioFormat = audioFormat;  // PCM (1) or IEEE float (3)
                header.numChannels = numChannels;
                header.sample_rate = sample_rate;
                header.bits_per_sample = bits_per_sample;
                header.byteRate = sample_rate * numChannels * bits_per_sample / 8;
                header.blockAlign = numChannels * bits_per_sample / 8;
                memcpy(header.dataID, "data", 4);
                header.data_size = 0;  // Will update on close

                fwrite(&header, sizeof(WavHeader), 1, file);

                // Remember position of data size field
                data_size_pos = sizeof(WavHeader) - sizeof(uint32_t);
                data_size = 0;

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

                data_size += size;
                return true;
            }

            /**
             * Close the file and update header with correct sizes
             */
            void Close()
            {
                if (!file) return;

                // Update data size
                fseek(file, data_size_pos, SEEK_SET);
                fwrite(&data_size, sizeof(uint32_t), 1, file);

                // Update file size
                uint32_t fileSize = data_size + sizeof(WavHeader) - 8;
                fseek(file, 4, SEEK_SET);
                fwrite(&fileSize, sizeof(uint32_t), 1, file);

                fclose(file);
                file = nullptr;
                data_size = 0;
            }
        };
    } // namespace audio
} // namespace hgl
