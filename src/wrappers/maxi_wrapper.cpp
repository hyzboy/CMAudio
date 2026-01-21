/**
 * @file maxi_wrapper.cpp
 * @brief C++ implementation of C ABI wrapper for Maximilian
 * 
 * This implementation provides a simple synthesizer using Maximilian's
 * oscillator and envelope classes, exposed through a C interface.
 * 
 * Design notes:
 * - Uses std::atomic for thread-safe parameter updates
 * - No heap allocations in audio callback path (maxi_process_float)
 * - Simple ADSR envelope for musical control
 * - Single oscillator design for minimal complexity
 * 
 * @copyright Copyright 2009 Mick Grierson (Maximilian)
 * @license MIT License
 */

#include "hgl/audio/maxi_wrapper.h"
#include "maximilian.h"
#include <atomic>
#include <cstring>
#include <cmath>

/**
 * @brief Internal synthesizer state
 * 
 * Contains Maximilian objects and atomic parameters for thread-safe updates
 */
struct MaxiHandle {
    // Maximilian objects
    maxiOsc oscillator;
    maxiEnv envelope;
    
    // Audio parameters (atomic for thread safety)
    std::atomic<float> frequency;
    std::atomic<float> gain;
    std::atomic<bool> trigger_pending;
    std::atomic<bool> release_pending;
    
    // Audio configuration
    int sample_rate;
    int num_channels;
    
    // Envelope settings
    float attack;
    float decay;
    float sustain;
    float release_time;
    
    // Envelope state
    bool envelope_active;
    
    /**
     * @brief Constructor
     */
    MaxiHandle(int sr, int channels)
        : frequency(440.0f)
        , gain(0.5f)
        , trigger_pending(false)
        , release_pending(false)
        , sample_rate(sr)
        , num_channels(channels)
        , attack(0.01)
        , decay(0.1)
        , sustain(0.7)
        , release_time(0.3)
        , envelope_active(false)
    {
        // Set Maximilian sample rate
        maxiSettings::sampleRate = sr;
        
        // Initialize envelope
        envelope.setAttack(attack * 1000.0);  // Convert to ms
        envelope.setDecay(decay * 1000.0);
        envelope.setSustain(sustain);
        envelope.setRelease(release_time * 1000.0);
    }
};

extern "C" {

MaxiHandle* maxi_create(int sample_rate, int num_channels) {
    // Validate parameters
    if (sample_rate <= 0 || sample_rate > 192000) {
        return nullptr;
    }
    if (num_channels <= 0 || num_channels > 8) {
        return nullptr;
    }
    
    try {
        MaxiHandle* handle = new MaxiHandle(sample_rate, num_channels);
        return handle;
    } catch (...) {
        return nullptr;
    }
}

void maxi_destroy(MaxiHandle* handle) {
    if (handle) {
        delete handle;
    }
}

int maxi_process_float(MaxiHandle* handle, float* output, int num_frames) {
    if (!handle || !output || num_frames <= 0) {
        return -1;
    }
    
    // Load atomic parameters once per buffer
    float freq = handle->frequency.load(std::memory_order_relaxed);
    float gain = handle->gain.load(std::memory_order_relaxed);
    
    // Check for envelope triggers
    if (handle->trigger_pending.exchange(false, std::memory_order_acquire)) {
        handle->envelope.trigger = 1;
        handle->envelope_active = true;
    }
    
    if (handle->release_pending.exchange(false, std::memory_order_acquire)) {
        handle->envelope.trigger = 0;
    }
    
    // Generate audio samples
    for (int frame = 0; frame < num_frames; ++frame) {
        // Generate oscillator sample
        double osc_sample = handle->oscillator.sinewave(freq);
        
        // Apply envelope
        double env_value = handle->envelope.adsr(1.0, handle->envelope.trigger);
        
        // Apply gain
        float sample = static_cast<float>(osc_sample * env_value * gain);
        
        // Clamp to valid range
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        
        // Write to all channels (interleaved)
        for (int ch = 0; ch < handle->num_channels; ++ch) {
            output[frame * handle->num_channels + ch] = sample;
        }
    }
    
    return num_frames;
}

void maxi_set_freq(MaxiHandle* handle, float frequency) {
    if (handle) {
        // Clamp to reasonable range
        if (frequency < 20.0f) frequency = 20.0f;
        if (frequency > 20000.0f) frequency = 20000.0f;
        handle->frequency.store(frequency, std::memory_order_relaxed);
    }
}

void maxi_set_gain(MaxiHandle* handle, float gain) {
    if (handle) {
        // Allow negative gain (phase inversion) but clamp extremes
        if (gain < -10.0f) gain = -10.0f;
        if (gain > 10.0f) gain = 10.0f;
        handle->gain.store(gain, std::memory_order_relaxed);
    }
}

float maxi_get_freq(MaxiHandle* handle) {
    if (handle) {
        return handle->frequency.load(std::memory_order_relaxed);
    }
    return 0.0f;
}

float maxi_get_gain(MaxiHandle* handle) {
    if (handle) {
        return handle->gain.load(std::memory_order_relaxed);
    }
    return 0.0f;
}

void maxi_trigger(MaxiHandle* handle) {
    if (handle) {
        handle->trigger_pending.store(true, std::memory_order_release);
    }
}

void maxi_release(MaxiHandle* handle) {
    if (handle) {
        handle->release_pending.store(true, std::memory_order_release);
    }
}

} // extern "C"
