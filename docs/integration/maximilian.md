# Maximilian Integration Guide

This document provides detailed information about the Maximilian audio library integration into CMAudio.

## Overview

Maximilian is integrated as a git submodule located at `third_party/Maximilian`. A C ABI wrapper (`maxi_wrapper`) provides a clean interface for C code to use Maximilian's C++ audio synthesis capabilities.

## Repository Structure

```
CMAudio/
├── third_party/
│   └── Maximilian/              # Submodule: Maximilian library
│       ├── LICENSE.txt          # MIT license
│       ├── src/                 # Maximilian source files
│       └── CMakeLists.txt       # Build configuration (created by us)
├── inc/hgl/audio/
│   └── maxi_wrapper.h           # C ABI header
├── src/wrappers/
│   ├── maxi_wrapper.cpp         # C++ implementation
│   └── CMakeLists.txt           # Wrapper build config
└── examples/maximilian_demo/
    ├── main_maxi.cpp            # Example usage
    └── CMakeLists.txt           # Example build config
```

## Building from Source

### Prerequisites

- CMake 3.15 or higher
- C++17 compatible compiler
- pthread library (Linux/macOS)

### Build Steps

1. Clone the repository with submodules:
```bash
git clone --recurse-submodules https://github.com/hyzboy/CMAudio.git
cd CMAudio
```

2. If you already cloned without submodules:
```bash
git submodule update --init --recursive
```

3. Configure and build:
```bash
cmake -S . -B build -DWITH_MAXIMILIAN=ON
cmake --build build
```

4. Run the example:
```bash
./build/bin/maximilian_demo
```

## CMake Options

### WITH_MAXIMILIAN (Default: ON)

Controls whether Maximilian support is built.

```bash
# Enable Maximilian (default)
cmake -S . -B build -DWITH_MAXIMILIAN=ON

# Disable Maximilian
cmake -S . -B build -DWITH_MAXIMILIAN=OFF
```

When disabled, the following will not be built:
- Maximilian library
- C wrapper (maximilian_wrapper)
- Example programs

## Updating the Submodule

To update Maximilian to the latest upstream version:

```bash
cd third_party/Maximilian
git fetch origin
git checkout master  # or specific tag/commit
cd ../..
git add third_party/Maximilian
git commit -m "Update Maximilian submodule"
```

## Architecture

### Design Choices

1. **Submodule vs Vendored Copy**
   - We use a git submodule to maintain a clear link to upstream
   - Allows easy updates and tracking of Maximilian versions
   - Preserves git history and licensing information

2. **C ABI Wrapper**
   - CMAudio is primarily C-based; Maximilian is C++
   - The wrapper provides a stable C interface
   - Uses `extern "C"` to prevent name mangling
   - Thread-safe parameter updates using std::atomic

3. **No Allocations in Audio Path**
   - All audio generation happens without heap allocations
   - Parameters are updated atomically from other threads
   - Maximilian objects are created once during initialization

### Wrapper Implementation Details

The wrapper (`maxi_wrapper.cpp`) provides:

- **MaxiHandle**: Opaque handle containing Maximilian oscillator and envelope
- **Atomic parameters**: Frequency and gain updates are thread-safe
- **Simple ADSR envelope**: Attack, decay, sustain, release control
- **Interleaved audio**: Generates stereo or mono output as needed

## Testing

### Running Tests

Currently, the integration is tested through the example program:

```bash
./build/bin/maximilian_demo
```

The example demonstrates:
1. Synthesizer creation and destruction
2. Parameter updates (frequency, gain)
3. Envelope triggering
4. Musical sequence generation

### Manual Verification

The demo outputs a musical scale and parameter sweeps. Success criteria:
- Program runs without crashing
- All notes are played in sequence
- Parameter demonstrations complete successfully

## Potential Issues and Solutions

### Issue: Missing system libraries

**Symptom:** CMake configuration fails with missing libvorbis or libopus

**Solution:** Install required audio libraries:
```bash
# Ubuntu/Debian
sudo apt-get install libvorbis-dev libopus-dev libogg-dev

# macOS
brew install libvorbis opus libogg

# Fedora/RHEL
sudo dnf install libvorbis-devel opus-devel libogg-devel
```

### Issue: Submodule not initialized

**Symptom:** `third_party/Maximilian` directory is empty

**Solution:** Initialize submodules:
```bash
git submodule update --init --recursive
```

### Issue: C++17 not supported

**Symptom:** Compiler errors about C++17 features

**Solution:** Update your compiler:
- GCC 7+ 
- Clang 5+
- MSVC 2017+

Or disable Maximilian:
```bash
cmake -S . -B build -DWITH_MAXIMILIAN=OFF
```

## Performance Considerations

- **CPU Usage**: Minimal - single oscillator with envelope
- **Memory**: ~1KB per synthesizer instance
- **Latency**: Depends on buffer size (example uses 2048 frames)

## Future Enhancements

Potential improvements for consideration:

1. **Multiple Oscillators**: Add support for multiple oscillators per handle
2. **More Envelope Types**: Expose other Maximilian envelope shapes
3. **Effects Chain**: Add filters, reverb, delay
4. **MIDI Support**: Integrate MIDI input for note triggering
5. **Polyphony**: Support multiple simultaneous notes
6. **CI Integration**: Add automated build and test workflows

## License

Maximilian is distributed under the MIT License:
- Copyright © 2009 Mick Grierson
- Full license text: `third_party/Maximilian/LICENSE.txt`

The C wrapper and integration code follows CMAudio's licensing terms.

## References

- **Maximilian Repository**: https://github.com/micknoise/Maximilian
- **Maximilian Documentation**: See `third_party/Maximilian/docs/`
- **CMAudio Repository**: https://github.com/hyzboy/CMAudio

## Maintainer Notes

### Code Review Checklist

Before merging changes:
- [ ] Submodule points to a stable commit
- [ ] Build succeeds with WITH_MAXIMILIAN=ON
- [ ] Build succeeds with WITH_MAXIMILIAN=OFF
- [ ] Example runs without errors
- [ ] No new warnings introduced
- [ ] Documentation is up to date

### Release Process

When releasing a new version of CMAudio with Maximilian:

1. Verify submodule commit
2. Update CHANGELOG with Maximilian version
3. Tag release with git tag
4. Document any API changes in the wrapper
