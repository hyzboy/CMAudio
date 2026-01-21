/**
 * @file main_maxi.cpp
 * @brief Example demonstrating Maximilian integration with CMAudio/OpenAL
 * 
 * This example shows how to:
 * 1. Create a Maximilian synthesizer using the C wrapper
 * 2. Generate audio samples
 * 3. Stream them to OpenAL for playback
 * 
 * The example plays a simple musical sequence with different frequencies.
 */

#include "hgl/audio/maxi_wrapper.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Simple OpenAL headers (if available, otherwise we'll simulate)
#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
// For Linux/Windows, we'll do a basic implementation without OpenAL
#endif

// Configuration
const int SAMPLE_RATE = 44100;
const int NUM_CHANNELS = 2;  // Stereo
const int BUFFER_SIZE = 2048;
const int NUM_BUFFERS = 4;
const float NOTE_DURATION_SECONDS = 0.3f;  // Duration for each note

/**
 * @brief Print audio information
 */
void print_info(MaxiHandle* synth) {
    std::cout << "=== Maximilian Demo ===" << std::endl;
    std::cout << "Sample Rate: " << SAMPLE_RATE << " Hz" << std::endl;
    std::cout << "Channels: " << NUM_CHANNELS << std::endl;
    std::cout << "Buffer Size: " << BUFFER_SIZE << " frames" << std::endl;
    std::cout << std::endl;
    std::cout << "Initial Frequency: " << maxi_get_freq(synth) << " Hz" << std::endl;
    std::cout << "Initial Gain: " << maxi_get_gain(synth) << std::endl;
    std::cout << std::endl;
}

/**
 * @brief Play a simple musical sequence
 */
void play_sequence(MaxiHandle* synth) {
    // Musical frequencies (C major scale)
    const float notes[] = {
        261.63f,  // C4
        293.66f,  // D4
        329.63f,  // E4
        349.23f,  // F4
        392.00f,  // G4
        440.00f,  // A4
        493.88f,  // B4
        523.25f   // C5
    };
    const int num_notes = sizeof(notes) / sizeof(notes[0]);
    
    std::cout << "Playing musical sequence..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;
    
    // Buffer for generated audio
    std::vector<float> buffer(BUFFER_SIZE * NUM_CHANNELS);
    
    // Play each note in sequence
    for (int iteration = 0; iteration < 2; ++iteration) {
        for (int i = 0; i < num_notes; ++i) {
            float freq = notes[i];
            std::cout << "Playing note " << (i + 1) << "/" << num_notes 
                      << " - " << freq << " Hz" << std::endl;
            
            // Set frequency
            maxi_set_freq(synth, freq);
            maxi_trigger(synth);
            
            // Generate and "play" audio for the specified note duration
            int num_buffers_per_note = (SAMPLE_RATE * NOTE_DURATION_SECONDS) / BUFFER_SIZE;
            for (int buf = 0; buf < num_buffers_per_note; ++buf) {
                // Generate audio
                int frames = maxi_process_float(synth, buffer.data(), BUFFER_SIZE);
                
                if (frames < 0) {
                    std::cerr << "Error generating audio!" << std::endl;
                    return;
                }
                
                // In a real application, this would be submitted to OpenAL
                // For this demo, we just simulate the timing
                std::this_thread::sleep_for(
                    std::chrono::microseconds(
                        (BUFFER_SIZE * 1000000) / SAMPLE_RATE
                    )
                );
            }
            
            maxi_release(synth);
            
            // Short pause between notes
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    
    std::cout << std::endl;
    std::cout << "Sequence complete!" << std::endl;
}

/**
 * @brief Demonstrate parameter changes
 */
void demonstrate_parameters(MaxiHandle* synth) {
    std::cout << "=== Parameter Demonstration ===" << std::endl;
    
    std::vector<float> buffer(BUFFER_SIZE * NUM_CHANNELS);
    
    // Test 1: Frequency sweep
    std::cout << "Test 1: Frequency sweep (200 Hz -> 800 Hz)" << std::endl;
    maxi_trigger(synth);
    
    for (int i = 0; i < 20; ++i) {
        float freq = 200.0f + (i / 20.0f) * 600.0f;
        maxi_set_freq(synth, freq);
        
        // Generate buffer
        maxi_process_float(synth, buffer.data(), BUFFER_SIZE);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    maxi_release(synth);
    std::cout << "  Complete!" << std::endl;
    std::cout << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Test 2: Gain changes
    std::cout << "Test 2: Gain modulation" << std::endl;
    maxi_set_freq(synth, 440.0f);
    maxi_trigger(synth);
    
    for (int i = 0; i < 20; ++i) {
        float gain = 0.5f * (1.0f + std::sin(i * 0.3f));
        maxi_set_gain(synth, gain);
        
        // Generate buffer
        maxi_process_float(synth, buffer.data(), BUFFER_SIZE);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    maxi_release(synth);
    std::cout << "  Complete!" << std::endl;
    std::cout << std::endl;
}

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
    std::cout << "============================================" << std::endl;
    std::cout << "  Maximilian + CMAudio Integration Demo" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << std::endl;
    
    // Create synthesizer
    MaxiHandle* synth = maxi_create(SAMPLE_RATE, NUM_CHANNELS);
    if (!synth) {
        std::cerr << "Failed to create Maximilian synthesizer!" << std::endl;
        return 1;
    }
    
    std::cout << "Synthesizer created successfully!" << std::endl;
    std::cout << std::endl;
    
    // Print information
    print_info(synth);
    
    // Run demonstrations
    try {
        demonstrate_parameters(synth);
        play_sequence(synth);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        maxi_destroy(synth);
        return 1;
    }
    
    // Cleanup
    maxi_destroy(synth);
    
    std::cout << std::endl;
    std::cout << "Demo complete! Synthesizer destroyed." << std::endl;
    std::cout << std::endl;
    std::cout << "Note: This demo simulates audio playback." << std::endl;
    std::cout << "In a real application, the generated samples would" << std::endl;
    std::cout << "be submitted to OpenAL or another audio backend." << std::endl;
    
    return 0;
}
