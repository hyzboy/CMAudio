# CMAudio v2 Release Notes (from CMAudioOLD)

## Overview

This release focuses on new capabilities rather than breaking API changes. The old version had limited API updates, while this version brings a new audio mixer, spatial audio, and expanded MIDI playback support. There are also a few lexical/syntax corrections in class names.

## Highlights

- New AudioMixer for offline and looped mixing workflows.
- Spatial audio pipeline and related utilities.
- Expanded MIDI playback stack with multiple plugin backends.
- Cleaner naming for manager classes and related headers.

## New Modules and Features

### Audio Mixer

- New `AudioMixer` and related types in `AudioMixerTypes`.
- Supports per-track time offset, volume, and pitch control.
- Configurable mixer settings via `MixerConfig`.
- Documentation: see [CMAudio/AudioMixer_Usage.md](../AudioMixer_Usage.md).

### Spatial Audio

- New spatial audio world system: `SpatialAudioWorld`.
- Added directional gain patterns via `DirectionalGainPattern`.
- Added reusable presets via `ReverbPreset`.
- New interpolation options in `InterpolationType`.

### MIDI Playback

- New MIDI stack: `MIDIPlayer`, `MIDIInstrument`, `MIDIOrchestraPlayer`.
- Multiple MIDI backends are now supported via plugins (see below).

### Memory and Manager Utilities

- New `AudioMemoryPool` for buffer allocation management.
- Manager renamed to `AudioManager` for consistency.

## API Naming Corrections (Lexical/Syntax Fixes)

- `AudioManage` -> `AudioManager`
  - Header: `AudioManage.h` replaced by `AudioManager.h`.
  - Source: `AudioManage.cpp` replaced by `AudioManager.cpp`.

> Note: No large-scale API redesign is intended. Most changes are additive, with a few naming fixes for clarity and correctness.

## Plugin Updates

### New Plugins

- `Audio.ADLMIDI`
- `Audio.FluidSynth`
- `Audio.OPNMIDI`
- `Audio.Timidity`
- `Audio.TinySoundFont`
- `Audio.WildMidi`

### Existing Plugins (still supported)

- `Audio.Opus`
- `Audio.Tremolo`
- `Audio.Tremor`
- `Audio.Vorbis`
- `Audio.Wav`
- `libogg`

## Header Additions (New Public API Surface)

- `AudioMixer.h`
- `AudioMixerTypes.h`
- `AudioManager.h`
- `AudioMemoryPool.h`
- `DirectionalGainPattern.h`
- `InterpolationType.h`
- `MIDIInstrument.h`
- `MIDIOrchestraPlayer.h`
- `MIDIPlayer.h`
- `ReverbPreset.h`
- `SpatialAudioWorld.h`

## Source Additions (Implementation)

- `AudioMixer.cpp`
- `AudioManager.cpp`
- `DirectionalGainPattern.cpp`
- `MIDIInstrument.cpp`
- `MIDIOrchestraPlayer.cpp`
- `MIDIPlayer.cpp`
- `ReverbPreset.cpp`
- `SpatialAudioWorld.cpp`

## Migration Notes

- If you used `AudioManage`, switch to `AudioManager` in both includes and class names.
- Existing playback, buffer, and scene APIs remain available. New features are additive.

## Recommended Next Steps

- Review and integrate the mixer usage guide.
- Choose an appropriate MIDI backend plugin based on target platform and license.
- Adopt the spatial audio utilities for directional and environment-based effects.
