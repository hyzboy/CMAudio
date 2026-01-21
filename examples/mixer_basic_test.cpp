// AudioMixer Basic Test
// Reads a single WAV file, creates multiple tracks with variations, and outputs mixed WAV
#include <iostream>
#include <hgl/audio/AudioMixer.h>
#include "WavReader.h"
#include "WavWriter.h"

using namespace hgl::audio;

int main(int argc, char** argv)
{
    const char* inputFile = (argc > 1) ? argv[1] : "wav_samples/car_small.wav";
    const char* outputFile = (argc > 2) ? argv[2] : "output_mixer_basic.wav";

    std::cout << "AudioMixer Basic Test" << std::endl;
    std::cout << "=====================" << std::endl;
    std::cout << "Input:  " << inputFile << std::endl;
    std::cout << "Output: " << outputFile << std::endl << std::endl;

    // Load input WAV file
    ALenum format;
    void* data;
    uint dataSize;
    uint sampleRate;

    if (!WavReader::Load(inputFile, &format, &data, &dataSize, &sampleRate))
    {
        std::cerr << "Error: Failed to load " << inputFile << std::endl;
        std::cerr << "Please ensure wav_samples directory contains the required files." << std::endl;
        return 1;
    }

    std::cout << "Loaded: " << dataSize << " bytes, ";
    std::cout << sampleRate << " Hz, ";
    std::cout << (format == AL_FORMAT_MONO8 ? "MONO8" : "MONO16") << std::endl;

    // Create mixer and set source
    AudioMixer mixer;
    mixer.SetSourceAudio(data, dataSize, format, sampleRate);
    mixer.SetOutputSampleRate(44100);  // Upsample to 44.1kHz
    
    // Optional: Configure mixer settings
    // MixerConfig config;
    // config.useSoftClipper = true;  // Use tanh-based soft clipping instead of hard clipping
    // config.useDither = true;       // Use TPDF dither when converting float32->int16 (reduces quantization noise)
    // mixer.SetConfig(config);
    
    // Add multiple tracks with different parameters
    std::cout << std::endl << "Adding tracks:" << std::endl;
    
    // Track 1: Original
    mixer.AddTrack(0.0f, 1.0f, 1.0f);
    std::cout << "  Track 1: offset=0.0s,  volume=1.0,  pitch=1.0   (original)" << std::endl;
    
    // Track 2: Slightly delayed, quieter, lower pitch
    mixer.AddTrack(0.5f, 0.7f, 0.95f);
    std::cout << "  Track 2: offset=0.5s,  volume=0.7,  pitch=0.95  (delayed, quieter)" << std::endl;
    
    // Track 3: More delay, medium volume, higher pitch
    mixer.AddTrack(1.2f, 0.6f, 1.05f);
    std::cout << "  Track 3: offset=1.2s,  volume=0.6,  pitch=1.05  (more delay)" << std::endl;
    
    // Track 4: Far away, very quiet, lower pitch
    mixer.AddTrack(2.0f, 0.4f, 0.9f);
    std::cout << "  Track 4: offset=2.0s,  volume=0.4,  pitch=0.9   (distant)" << std::endl;

    // Mix audio (5 seconds)
    void* outputData;
    uint outputSize;
    
    std::cout << std::endl << "Mixing audio (5 seconds)..." << std::endl;
    if (!mixer.Mix(&outputData, &outputSize, 5.0f))
    {
        std::cerr << "Error: Failed to mix audio" << std::endl;
        free(data);
        return 1;
    }

    std::cout << "Mixed: " << outputSize << " bytes" << std::endl;

    // Write output WAV file
    WavWriter writer;
    if (!writer.Open(outputFile, AL_FORMAT_MONO16, 44100))
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
