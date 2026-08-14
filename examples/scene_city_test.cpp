// AudioMixerScene City Scene Test
// Loads city_scene.toml configuration and generates a mixed urban environment
#include <iostream>
#include <map>
#include <hgl/audio/AudioMixerScene.h>
#include "AudioMixerSceneConfig.h"
#include "WavReader.h"
#include "WavWriter.h"

using namespace hgl;
using namespace hgl::audio;

int main(int argc, char** argv)
{
    const char* configFile = (argc > 1) ? argv[1] : "configs/city_scene.toml";
    const char* outputFile = (argc > 2) ? argv[2] : "output_city_scene.wav";

    std::cout << "AudioMixerScene City Scene Test" << std::endl;
    std::cout << "==========================" << std::endl;
    std::cout << "Config: " << configFile << std::endl;
    std::cout << "Output: " << outputFile << std::endl << std::endl;

    // Load TOML configuration
    SceneConfigData config;
    if (!AudioMixerSceneConfig::Load(configFile, config))
    {
        std::cerr << "Error: Failed to load configuration file" << std::endl;
        return 1;
    }

    std::cout << "Configuration loaded:" << std::endl;
    std::cout << "  Duration: " << config.duration << " seconds" << std::endl;
    std::cout << "  Output: " << config.output.info.sample_rate << " Hz, ";
    std::cout << (config.output.info.bits_per_sample == 16 ? "MONO16" : "MONO8") << std::endl;
    std::cout << "  Sources: " << config.sources.size() << std::endl << std::endl;

    // Load all WAV files
    std::map<std::string, void*> wavData;
    std::map<std::string, uint> wavSize;
    std::map<std::string, openal::ALenum> wavFormat;
    std::map<std::string, uint> wavSampleRate;

    std::cout << "Loading WAV files:" << std::endl;
    for (const auto& source : config.sources)
    {
        openal::ALenum format;
        void* data;
        uint data_size;
        uint sample_rate;

        std::cout << "  " << source.wavFile << "... ";
        if (!WavReader::Load(source.wavFile.c_str(), &format, &data, &data_size, &sample_rate))
        {
            std::cerr << "FAILED" << std::endl;
            std::cerr << "Error: Failed to load " << source.wavFile << std::endl;

            // Cleanup already loaded files
            for (auto& pair : wavData)
                free(pair.second);

            return 1;
        }

        wavData[source.wavFile] = data;
        wavSize[source.wavFile] = data_size;
        wavFormat[source.wavFile] = format;
        wavSampleRate[source.wavFile] = sample_rate;

        std::cout << "OK (" << data_size << " bytes, " << sample_rate << " Hz)" << std::endl;
    }

    // Create AudioMixerScene
    AudioMixerScene scene;
    scene.SetOutputFormat(config.output.info);

    std::cout << std::endl << "Configuring sources:" << std::endl;
    for (const auto& source : config.sources)
    {
        AudioMixerSourceConfig srcConfig;
        srcConfig.data = wavData[source.wavFile];
        srcConfig.info.data_size = wavSize[source.wavFile];
        srcConfig.info.channels = (wavFormat[source.wavFile] == AL_FORMAT_STEREO16 || wavFormat[source.wavFile] == AL_FORMAT_STEREO8 || wavFormat[source.wavFile] == AL_FORMAT_STEREO_FLOAT32) ? 2 : 1;
        srcConfig.info.bits_per_sample = (wavFormat[source.wavFile] == AL_FORMAT_MONO8 || wavFormat[source.wavFile] == AL_FORMAT_STEREO8) ? 8 : 16;
        srcConfig.info.is_float = (wavFormat[source.wavFile] == AL_FORMAT_MONO_FLOAT32 || wavFormat[source.wavFile] == AL_FORMAT_STEREO_FLOAT32);
        srcConfig.info.sample_rate = wavSampleRate[source.wavFile];
        srcConfig.min_count = source.min_count;
        srcConfig.max_count = source.max_count;
        srcConfig.min_interval = source.min_interval;
        srcConfig.max_interval = source.max_interval;
        srcConfig.min_volume = source.min_volume;
        srcConfig.max_volume = source.max_volume;
        srcConfig.min_pitch = source.min_pitch;
        srcConfig.max_pitch = source.max_pitch;

        // Convert name to UTF-16
        std::u16string u16name;
        for (char c : source.name)
            u16name.push_back((char16_t)c);

        scene.AddSource((const os_char*)u16name.c_str(), srcConfig);

        std::cout << "  " << source.name << ": ";
        std::cout << source.min_count << "-" << source.max_count << " instances, ";
        std::cout << "volume " << source.min_volume << "-" << source.max_volume << ", ";
        std::cout << "pitch " << source.min_pitch << "-" << source.max_pitch << std::endl;
    }

    // Generate scene
    void* outputData;
    uint outputSize;

    std::cout << std::endl << "Generating scene (" << config.duration << " seconds)..." << std::endl;
    if (!scene.GenerateScene(&outputData, &outputSize, config.duration))
    {
        std::cerr << "Error: Failed to generate scene" << std::endl;

        // Cleanup
        for (auto& pair : wavData)
            free(pair.second);

        return 1;
    }

    std::cout << "Generated: " << outputSize << " bytes" << std::endl;

    // Write output WAV file
    WavWriter writer;
    if (!writer.Open(outputFile, openal::ToOpenALFormat(config.output.info), config.output.info.sample_rate))
    {
        std::cerr << "Error: Failed to create output file" << std::endl;
        delete[] (char*)outputData;

        // Cleanup
        for (auto& pair : wavData)
            free(pair.second);

        return 1;
    }

    writer.Write(outputData, outputSize);
    writer.Close();

    std::cout << "Output written to: " << outputFile << std::endl;
    std::cout << std::endl << "Test completed successfully!" << std::endl;

    // Cleanup
    delete[] (char*)outputData;
    for (auto& pair : wavData)
        free(pair.second);

    return 0;
}
