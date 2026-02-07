// AudioMixerScene City Scene Test
// Loads city_scene.toml configuration and generates a mixed urban environment
#include <iostream>
#include <map>
#include <hgl/audio/AudioMixerScene.h>
#include "AudioMixerSceneConfig.h"
#include "WavReader.h"
#include "WavWriter.h"

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
    std::cout << "  Output: " << config.output.sampleRate << " Hz, ";
    std::cout << (config.output.format == AL_FORMAT_MONO16 ? "MONO16" : "MONO8") << std::endl;
    std::cout << "  Sources: " << config.sources.size() << std::endl << std::endl;

    // Load all WAV files
    std::map<std::string, void*> wavData;
    std::map<std::string, uint> wavSize;
    std::map<std::string, ALenum> wavFormat;
    std::map<std::string, uint> wavSampleRate;

    std::cout << "Loading WAV files:" << std::endl;
    for (const auto& source : config.sources)
    {
        ALenum format;
        void* data;
        uint dataSize;
        uint sampleRate;

        std::cout << "  " << source.wavFile << "... ";
        if (!WavReader::Load(source.wavFile.c_str(), &format, &data, &dataSize, &sampleRate))
        {
            std::cerr << "FAILED" << std::endl;
            std::cerr << "Error: Failed to load " << source.wavFile << std::endl;

            // Cleanup already loaded files
            for (auto& pair : wavData)
                free(pair.second);

            return 1;
        }

        wavData[source.wavFile] = data;
        wavSize[source.wavFile] = dataSize;
        wavFormat[source.wavFile] = format;
        wavSampleRate[source.wavFile] = sampleRate;

        std::cout << "OK (" << dataSize << " bytes, " << sampleRate << " Hz)" << std::endl;
    }

    // Create AudioMixerScene
    AudioMixerScene scene;
    scene.SetOutputFormat(config.output.format, config.output.sampleRate);

    std::cout << std::endl << "Configuring sources:" << std::endl;
    for (const auto& source : config.sources)
    {
        AudioMixerSourceConfig srcConfig;
        srcConfig.data = wavData[source.wavFile];
        srcConfig.dataSize = wavSize[source.wavFile];
        srcConfig.format = wavFormat[source.wavFile];
        srcConfig.sampleRate = wavSampleRate[source.wavFile];
        srcConfig.minCount = source.minCount;
        srcConfig.maxCount = source.maxCount;
        srcConfig.minInterval = source.minInterval;
        srcConfig.maxInterval = source.maxInterval;
        srcConfig.minVolume = source.minVolume;
        srcConfig.maxVolume = source.maxVolume;
        srcConfig.minPitch = source.minPitch;
        srcConfig.maxPitch = source.maxPitch;

        // Convert name to UTF-16
        std::u16string u16name;
        for (char c : source.name)
            u16name.push_back((char16_t)c);

        scene.AddSource((const os_char*)u16name.c_str(), srcConfig);

        std::cout << "  " << source.name << ": ";
        std::cout << source.minCount << "-" << source.maxCount << " instances, ";
        std::cout << "volume " << source.minVolume << "-" << source.maxVolume << ", ";
        std::cout << "pitch " << source.minPitch << "-" << source.maxPitch << std::endl;
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
    if (!writer.Open(outputFile, config.output.format, config.output.sampleRate))
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
