# AudioMixer Examples

This directory contains example programs demonstrating the AudioMixer and AudioScene systems.

## Overview

Three example programs are provided:

1. **mixer_basic_test** - Basic AudioMixer demo (single source, multiple tracks)
2. **scene_city_test** - AudioScene demo with city environment (TOML config)
3. **scene_swarm_test** - AudioScene demo with bee swarm (TOML config)

## Key Features

### Float-Based Internal Mixing
- **All mixing operations use float internally** (-1.0 to 1.0 range)
- **No clipping distortion** when mixing multiple tracks
- **High precision** - maintains quality with many overlapping sounds
- **Flexible output**: Can output as int16 or float32

### Soft Clipper
- **Soft Clipper algorithm**: Optional tanh-based soft clipping for float32 data
- **Smooth distortion**: When audio exceeds [-1.0, 1.0], applies gradual compression instead of harsh cutoff
- **Toggle on/off**: Use `MixerConfig::useSoftClipper` to enable/disable
- **Best for**: High-density mixes (10+ tracks) where peaks may exceed normal range
- **Comparison**:
  - **Hard clipping** (default): Abruptly cuts peaks at ±1.0 (can sound harsh)
  - **Normalize**: Scales entire mix to fit (changes overall volume)
  - **Soft clipping**: Gradually compresses peaks (more musical, preserves dynamic range)

### Dither (NEW)
- **TPDF Dither**: Optional dithering when converting float32 to int16
- **Reduces quantization noise**: Adds shaped noise to mask rounding errors
- **Toggle on/off**: Use `MixerConfig::useDither` to enable/disable
- **Why use it**:
  - Float32 has ~144dB dynamic range, int16 only has ~96dB
  - Direct conversion causes audible quantization distortion in quiet passages
  - Dither randomizes quantization errors, making them sound like gentle noise instead of harsh artifacts
- **When to use**: 
  - Enable for high-quality output where subtle details matter
  - Disable for game audio where small noise is acceptable
- **TPDF (Triangular PDF)**: Industry-standard dither type, sounds more natural than uniform noise

### Benefits
- Better quality when mixing 10+ tracks
- No accumulation of rounding errors
- Industry-standard approach (FMOD, Wwise, Unity all use float internally)
- Soft clipper provides natural-sounding peak limiting

## Prerequisites

### Required WAV Files

Before running the examples, you need to provide WAV audio samples. See `wav_samples/README.md` for:
- List of required files
- Format specifications (mono, 16-bit, 22050/44100 Hz)
- Where to download free samples
- How to convert existing files

### Building

The examples are built automatically when you build the main CMAudio project:

```bash
cd build
cmake ..
make
```

Or disable examples:

```bash
cmake .. -DBUILD_EXAMPLES=OFF
make
```

## Example Programs

### 1. mixer_basic_test

**Description**: Demonstrates basic AudioMixer functionality by loading a single WAV file and creating 4 overlapping tracks with different parameters.

**Usage**:
```bash
./mixer_basic_test [input.wav] [output.wav]

# Examples:
./mixer_basic_test                                    # Uses default files
./mixer_basic_test wav_samples/car_small.wav output.wav
```

**What it does**:
- Loads a single audio source (default: car_small.wav)
- Creates 4 tracks with variations:
  - Track 1: Original (0s offset, 100% volume, 1.0x pitch)
  - Track 2: Delayed (0.5s offset, 70% volume, 0.95x pitch)
  - Track 3: More delayed (1.2s offset, 60% volume, 1.05x pitch)
  - Track 4: Distant (2.0s offset, 40% volume, 0.9x pitch)
- Mixes all tracks into 5 seconds of audio
- Outputs to WAV file (44.1kHz, mono, 16-bit)

**Output**: `output_mixer_basic.wav`

### 2. scene_city_test

**Description**: Demonstrates AudioScene with a complex city street environment using TOML configuration.

**Usage**:
```bash
./scene_city_test [config.toml] [output.wav]

# Examples:
./scene_city_test                                     # Uses default config
./scene_city_test configs/city_scene.toml my_city.wav
```

**What it does**:
- Loads `configs/city_scene.toml` configuration
- Loads multiple WAV files (cars, horns, birds, etc.)
- Randomly generates instances based on configuration:
  - Small cars: 2-4 instances with 1-3s intervals
  - SUVs: 1-2 instances with 2-5s intervals
  - Trucks: 0-1 instances with 3-8s intervals
  - Horns: 0-3 short, 0-1 long with random timing
  - Birds: 2-5 instances for background ambience
  - Brakes: 0-2 instances rarely
- Mixes all sources into 30 seconds of city audio
- Outputs to WAV file

**Output**: `output_city_scene.wav`

**Configuration**: Edit `configs/city_scene.toml` to customize:
- Number of instances per source type
- Time intervals between sounds
- Volume and pitch ranges
- Output duration and sample rate

### 3. scene_swarm_test

**Description**: Demonstrates AudioScene with high-density swarm using TOML configuration.

**Usage**:
```bash
./scene_swarm_test [config.toml] [output.wav]

# Examples:
./scene_swarm_test                                    # Uses default config
./scene_swarm_test configs/swarm_scene.toml my_swarm.wav
```

**What it does**:
- Loads `configs/swarm_scene.toml` configuration
- Loads bee_buzz.wav
- Creates 10-15 overlapping instances with:
  - Varying volume (0.4-0.8)
  - Varying pitch (0.9-1.15) to simulate different wing frequencies
  - Minimal time offsets (0-0.5s) for dense sound
- Mixes into 10 seconds of realistic swarm audio
- Outputs to WAV file

**Output**: `output_swarm_scene.wav`

**Key concept**: Instead of mixing hundreds of individual bees, the system creates a realistic swarm effect with only 10-15 carefully varied instances.

## TOML Configuration Format

AudioScene examples use TOML files for configuration. Example structure:

```toml
[output]
format = "MONO16"
sample_rate = 44100

[scene]
duration = 30.0

[source.car]
wav_file = "wav_samples/car_small.wav"
min_count = 2
max_count = 4
min_interval = 1.0
max_interval = 3.0
min_volume = 0.6
max_volume = 1.0
min_pitch = 0.95
max_pitch = 1.05
```

### Configuration Parameters

**[output] section**:
- `format`: "MONO8" or "MONO16"
- `sample_rate`: Output sample rate (e.g., 44100, 48000)

**[scene] section**:
- `duration`: Total duration in seconds

**[source.NAME] sections**:
- `wav_file`: Path to source WAV file
- `min_count` / `max_count`: Range of instances to generate
- `min_interval` / `max_interval`: Time between instances (seconds)
- `min_volume` / `max_volume`: Volume range (0.0-1.0)
- `min_pitch` / `max_pitch`: Pitch range (0.5-2.0, typically 0.9-1.1)

## File Structure

```
examples/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
│
├── WavReader.h                 # WAV file reader utility
├── WavWriter.h                 # WAV file writer utility
├── AudioSceneConfig.h          # TOML configuration parser
│
├── mixer_basic_test.cpp        # Basic mixer example
├── scene_city_test.cpp         # City scene example
├── scene_swarm_test.cpp        # Swarm example
│
├── configs/
│   ├── city_scene.toml         # City configuration
│   └── swarm_scene.toml        # Swarm configuration
│
└── wav_samples/
    ├── README.md               # WAV files guide
    ├── car_small.wav           # (you provide)
    ├── car_suv.wav             # (you provide)
    ├── car_truck.wav           # (you provide)
    ├── horn_short.wav          # (you provide)
    ├── horn_long.wav           # (you provide)
    ├── brake_screech.wav       # (you provide)
    ├── bird_chirp.wav          # (you provide)
    └── bee_buzz.wav            # (you provide)
```

## Troubleshooting

### "Failed to load WAV file"
- Ensure WAV files exist in `wav_samples/` directory
- Check that files are mono, not stereo
- Verify files are PCM format (not compressed)

### "Failed to generate scene"
- Check TOML configuration syntax
- Ensure all referenced WAV files exist
- Verify file paths in config are correct

### Build errors
- Ensure CMAudio library is built first
- Check that all headers are accessible
- Verify cmake configuration succeeded

## Creating Custom Configurations

To create your own scene:

1. Copy an existing TOML file (e.g., `city_scene.toml`)
2. Modify source sections to match your WAV files
3. Adjust parameters (counts, intervals, volumes, pitches)
4. Run the test program with your config:
   ```bash
   ./scene_city_test my_config.toml my_output.wav
   ```

## Tips

- **Looping sounds**: Use seamless loops for continuous sounds (engines, buzzing)
- **Volume balance**: Keep total volume under 1.0 to avoid clipping
- **Pitch variation**: Small pitch changes (0.9-1.1) add realism
- **Instance counts**: More instances = denser sound but higher CPU
- **Intervals**: Shorter intervals = more frequent sounds

## Performance

- Each example runs in real-time on modern CPUs
- City scene (7 sources, 30s): typically < 1 second to generate
- Swarm (1 source, 10s): typically < 0.5 seconds to generate
- Output files are typically 1-3 MB depending on duration

## Next Steps

After trying the examples:

1. Experiment with different TOML configurations
2. Try your own WAV files
3. Create custom scenes for your use case
4. Integrate AudioMixer/AudioScene into your application
5. See main documentation for API details

## License

These examples are part of the CMAudio project and follow the same license.
