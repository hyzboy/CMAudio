# CMAudio

An audio library with OpenAL support and audio codec plugins.

## Third-party: Maximilian

CMAudio integrates [Maximilian](https://github.com/micknoise/Maximilian), a powerful C++ audio synthesis and signal processing library, as an optional component.

**License:** MIT (see [third_party/Maximilian/LICENSE.txt](third_party/Maximilian/LICENSE.txt))

**Features provided:**
- Audio synthesis (oscillators, filters, envelopes)
- Real-time effects processing
- C ABI wrapper for easy integration with C code

### Building with Maximilian

By default, Maximilian support is **enabled**. To build:

```bash
# With Maximilian (default)
cmake -S . -B build -DWITH_MAXIMILIAN=ON
cmake --build build

# Without Maximilian
cmake -S . -B build -DWITH_MAXIMILIAN=OFF
cmake --build build
```

### Example Usage

See `examples/maximilian_demo/` for a complete example demonstrating:
- Creating a Maximilian synthesizer
- Generating audio samples
- Parameter modulation (frequency, gain)
- Musical sequence playback

Run the example:
```bash
./build/bin/maximilian_demo
```

### C API

The C wrapper API is defined in `inc/hgl/audio/maxi_wrapper.h`. Key functions:

```c
// Create/destroy synthesizer
MaxiHandle* maxi_create(int sample_rate, int num_channels);
void maxi_destroy(MaxiHandle* handle);

// Generate audio
int maxi_process_float(MaxiHandle* handle, float* output, int num_frames);

// Control parameters
void maxi_set_freq(MaxiHandle* handle, float frequency);
void maxi_set_gain(MaxiHandle* handle, float gain);
void maxi_trigger(MaxiHandle* handle);
void maxi_release(MaxiHandle* handle);
```

### Attribution

Maximilian is developed by Mick Grierson and contributors. See the [upstream repository](https://github.com/micknoise/Maximilian) for more information.

