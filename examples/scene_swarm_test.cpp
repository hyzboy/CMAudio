// AudioScene Swarm Test
// Loads swarm_scene.toml configuration and generates a bee swarm sound
#include <iostream>
#include <hgl/audio/AudioScene.h>
#include "AudioSceneConfig.h"
#include "WavReader.h"
#include "WavWriter.h"

using namespace hgl::audio;

int main(int argc, char** argv)
{
    const char* configFile = (argc > 1) ? argv[1] : "configs/swarm_scene.toml";
    const char* outputFile = (argc > 2) ? argv[2] : "output_swarm_scene.wav";

    std::cout << "AudioScene Swarm Test" << std::endl;
    std::cout << "=====================" << std::endl;
    std::cout << "Config: " << configFile << std::endl;
    std::cout << "Output: " << outputFile << std::endl << std::endl;

    // Load TOML configuration
    SceneConfigData config;
    if (!AudioSceneConfig::Load(configFile, config))
    {
        std::cerr << "Error: Failed to load configuration file" << std::endl;
        return 1;
    }

    std::cout << "Configuration loaded:" << std::endl;
    std::cout << "  Duration: " << config.duration << " seconds" << std::endl;
    std::cout << "  Output: " << config.output.sampleRate << " Hz, ";
    std::cout << (config.output.format == AL_FORMAT_MONO16 ? "MONO16" : "MONO8") << std::endl;
    std::cout << "  Sources: " << config.sources.size() << std::endl << std::endl;

    // For swarm, we should only have one source type
    if (config.sources.empty())
    {
        std::cerr << "Error: No sources configured" << std::endl;
        return 1;
    }

    const auto& source = config.sources[0];

    // Load WAV file
    ALenum format;
    void* data;
    uint dataSize;
    uint sampleRate;

    std::cout << "Loading: " << source.wavFile << "... ";
    if (!WavReader::Load(source.wavFile.c_str(), &format, &data, &dataSize, &sampleRate))
    {
        std::cerr << "FAILED" << std::endl;
        std::cerr << "Error: Failed to load " << source.wavFile << std::endl;
        return 1;
    }
    std::cout << "OK (" << dataSize << " bytes, " << sampleRate << " Hz)" << std::endl;

    // Create AudioScene
    AudioScene scene;
    scene.SetOutputFormat(config.output.format, config.output.sampleRate);

    AudioSourceConfig srcConfig;
    srcConfig.data = data;
    srcConfig.dataSize = dataSize;
    srcConfig.format = format;
    srcConfig.sampleRate = sampleRate;
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

    std::cout << std::endl << "Swarm configuration:" << std::endl;
    std::cout << "  Name: " << source.name << std::endl;
    std::cout << "  Instances: " << source.minCount << "-" << source.maxCount << std::endl;
    std::cout << "  Volume: " << source.minVolume << "-" << source.maxVolume << std::endl;
    std::cout << "  Pitch: " << source.minPitch << "-" << source.maxPitch << std::endl;
    std::cout << "  (Simulates high-density swarm with " << source.minCount << "-" << source.maxCount << " mixed instances)" << std::endl;

    // Generate scene
    void* outputData;
    uint outputSize;

    std::cout << std::endl << "Generating swarm sound (" << config.duration << " seconds)..." << std::endl;
    if (!scene.GenerateScene(&outputData, &outputSize, config.duration))
    {
        std::cerr << "Error: Failed to generate scene" << std::endl;
        free(data);
        return 1;
    }

    std::cout << "Generated: " << outputSize << " bytes" << std::endl;

    // Write output WAV file
    WavWriter writer;
    if (!writer.Open(outputFile, config.output.format, config.output.sampleRate))
    {
        std::cerr << "Error: Failed to create output file" << std::endl;
        delete[] (char*)outputData;
        free(data);
        return 1;
    }

    writer.Write(outputData, outputSize);
    writer.Close();

    std::cout << "Output written to: " << outputFile << std::endl;
    std::cout << std::endl << "Test completed successfully!" << std::endl;

    // Cleanup
    delete[] (char*)outputData;
    free(data);

    return 0;
}
