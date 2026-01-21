/**
 * @file maxi_wrapper.h
 * @brief C ABI wrapper for Maximilian audio synthesis library
 * 
 * This header provides a C-compatible interface to the Maximilian C++ audio library.
 * It allows C code (like CMAudio) to use Maximilian's synthesis capabilities.
 * 
 * Example usage:
 * @code
 * MaxiHandle* synth = maxi_create(44100, 2);
 * maxi_set_freq(synth, 440.0f);
 * maxi_set_gain(synth, 0.5f);
 * 
 * float buffer[512];
 * maxi_process_float(synth, buffer, 256);
 * 
 * maxi_destroy(synth);
 * @endcode
 * 
 * @note All functions are thread-safe for parameter updates but not for concurrent
 * calls to maxi_process_float on the same handle.
 * 
 * @copyright Copyright 2009 Mick Grierson (Maximilian)
 * @license MIT License - see third_party/Maximilian/LICENSE.txt
 */

#ifndef HGL_AUDIO_MAXI_WRAPPER_H
#define HGL_AUDIO_MAXI_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle to a Maximilian synthesizer instance
 */
typedef struct MaxiHandle MaxiHandle;

/**
 * @brief Create a new Maximilian synthesizer instance
 * 
 * @param sample_rate Sample rate in Hz (e.g., 44100, 48000)
 * @param num_channels Number of audio channels (1 for mono, 2 for stereo)
 * @return Pointer to synthesizer instance, or NULL on failure
 * 
 * @note The returned handle must be freed with maxi_destroy()
 */
MaxiHandle* maxi_create(int sample_rate, int num_channels);

/**
 * @brief Destroy a Maximilian synthesizer instance
 * 
 * @param handle Synthesizer instance to destroy
 * 
 * @note After calling this function, the handle is invalid and must not be used
 */
void maxi_destroy(MaxiHandle* handle);

/**
 * @brief Process audio and fill output buffer with synthesized samples
 * 
 * @param handle Synthesizer instance
 * @param output Output buffer for interleaved float samples
 * @param num_frames Number of frames to generate (not samples)
 *                   For stereo, generates num_frames*2 samples
 * 
 * @return Number of frames actually generated, or -1 on error
 * 
 * @note This function should be called from the audio callback thread
 * @note Buffer must be large enough: num_frames * num_channels * sizeof(float)
 * @note Output range is [-1.0, 1.0]
 */
int maxi_process_float(MaxiHandle* handle, float* output, int num_frames);

/**
 * @brief Set the oscillator frequency
 * 
 * @param handle Synthesizer instance
 * @param frequency Frequency in Hz (e.g., 440.0 for A4)
 * 
 * @note Thread-safe: can be called from any thread
 * @note Changes take effect on the next call to maxi_process_float()
 */
void maxi_set_freq(MaxiHandle* handle, float frequency);

/**
 * @brief Set the output gain/amplitude
 * 
 * @param handle Synthesizer instance
 * @param gain Amplitude multiplier (0.0 = silence, 1.0 = full volume)
 * 
 * @note Thread-safe: can be called from any thread
 * @note Recommended range: [0.0, 1.0], but higher values are allowed
 */
void maxi_set_gain(MaxiHandle* handle, float gain);

/**
 * @brief Get current oscillator frequency
 * 
 * @param handle Synthesizer instance
 * @return Current frequency in Hz
 */
float maxi_get_freq(MaxiHandle* handle);

/**
 * @brief Get current output gain
 * 
 * @param handle Synthesizer instance
 * @return Current gain value
 */
float maxi_get_gain(MaxiHandle* handle);

/**
 * @brief Trigger the envelope (note on)
 * 
 * @param handle Synthesizer instance
 * 
 * @note Use this to create percussive or gated sounds
 */
void maxi_trigger(MaxiHandle* handle);

/**
 * @brief Release the envelope (note off)
 * 
 * @param handle Synthesizer instance
 * 
 * @note Use this to stop sustained notes
 */
void maxi_release(MaxiHandle* handle);

#ifdef __cplusplus
}
#endif

#endif /* HGL_AUDIO_MAXI_WRAPPER_H */
