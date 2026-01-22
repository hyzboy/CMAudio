# MIDI Configuration Interface API

**CMAudio MIDI Plugin Configuration Guide**

This document describes the MIDI configuration interface added to CMAudio, allowing runtime customization of MIDI synthesis parameters across all 6 MIDI plugins.

## Overview

The MIDI configuration interface (version 4) provides a unified API to configure MIDI synthesis parameters such as soundfont/bank paths, volume, sample rate, polyphony, and effects. All 6 MIDI plugins (FluidSynth, TinySoundFont, WildMIDI, Timidity, OPNMIDI, ADLMIDI) implement this interface.

## Interface Definition

Located in `src/AudioDecode.h`:

```cpp
struct AudioMidiConfigInterface
{
    void    (*SetSoundFont      )(const char *);      // Set soundfont/bank file path
    void    (*SetBankID         )(int);               // Set bank ID (chip emulators)
    void    (*SetVolume         )(float);             // Set global volume (0.0-1.0)
    void    (*SetSampleRate     )(int);               // Set sample rate (default: 44100)
    void    (*SetPolyphony      )(int);               // Set max polyphony/voices
    void    (*SetChipCount      )(int);               // Set number of emulated chips
    void    (*EnableReverb      )(bool);              // Enable/disable reverb
    void    (*EnableChorus      )(bool);              // Enable/disable chorus
    const char* (*GetVersion    )();                  // Get plugin version string
    const char* (*GetDefaultBank)();                  // Get default bank/soundfont path
};
```

## Usage

### 1. Load the Plugin and Get Interface

```cpp
#include "src/AudioDecode.h"

// Get the MIDI configuration interface
AudioMidiConfigInterface midi_config;
if (GetAudioMidiInterface(OS_TEXT("MIDI"), &midi_config))
{
    // Configuration successful
}
```

### 2. Configure Before Playing

#### FluidSynth Example

```cpp
// Set high-quality soundfont
midi_config.SetSoundFont("/usr/share/sounds/sf2/GeneralUser_GS.sf2");

// Professional settings
midi_config.SetSampleRate(48000);      // High quality sample rate
midi_config.SetPolyphony(512);         // More simultaneous notes
midi_config.SetVolume(0.8f);           // 80% volume
midi_config.EnableReverb(true);        // Spacious sound
midi_config.EnableChorus(true);        // Rich sound

// Now play MIDI
AudioPlayer midi;
midi.Load("music.mid");
midi.Play(true);
```

#### TinySoundFont Example

```cpp
// Use lightweight soundfont
midi_config.SetSoundFont("/usr/share/sounds/sf2/FluidR3_GM.sf2");
midi_config.SetVolume(0.9f);

// TinySoundFont doesn't support reverb/chorus (no-op)
midi_config.EnableReverb(true);  // Ignored
```

#### OPNMIDI (Genesis/YM2612) Example

```cpp
// Chip emulator uses banks, not soundfonts
midi_config.SetBankID(74);             // Use bank #74 (Sonic 3 sound)
midi_config.SetChipCount(4);           // 4 chips = 24 channels (6*4)
midi_config.SetVolume(1.0f);           // Full volume
midi_config.SetSampleRate(44100);      // Standard rate

// SoundFont and effects settings are no-ops for chip emulators
midi_config.SetSoundFont("ignored");   // No effect
midi_config.EnableReverb(true);        // No effect
```

#### ADLMIDI (AdLib/OPL3) Example

```cpp
// Classic DOS game sound
midi_config.SetBankID(0);              // DMX OP2 (Doom sound)
midi_config.SetChipCount(2);           // 2 OPL3 chips = 36 channels (18*2)
midi_config.SetVolume(0.85f);          // Slightly reduced volume
```

### 3. Query Plugin Information

```cpp
// Get version and default paths
const char* version = midi_config.GetVersion();
const char* default_bank = midi_config.GetDefaultBank();

printf("Using: %s\n", version);
printf("Default bank: %s\n", default_bank);
```

## Configuration Parameters

### SetSoundFont(const char* path)

**Applies to:** FluidSynth, TinySoundFont, WildMIDI, Timidity  
**Purpose:** Set custom SoundFont (.sf2) or patch file path  
**Priority:** API setting > Environment variable > Default path

```cpp
// High quality SoundFont
midi_config.SetSoundFont("/usr/share/sounds/sf2/GeneralUser_GS.sf2");

// Or use environment variable instead:
// export FLUIDSYNTH_SF2=/path/to/soundfont.sf2
```

### SetBankID(int bank_id)

**Applies to:** OPNMIDI, ADLMIDI  
**Purpose:** Select embedded instrument bank for chip emulation

```cpp
// OPNMIDI banks (0-73+)
midi_config.SetBankID(0);   // Bank 0: Default OPNMIDI bank
midi_config.SetBankID(74);  // Bank 74: Sonic 3 instruments

// ADLMIDI banks (0-71+)
midi_config.SetBankID(0);   // Bank 0: DMX OP2 (Doom, Heretic, Hexen)
midi_config.SetBankID(14);  // Bank 14: WOPL bank
```

### SetVolume(float volume)

**Applies to:** All plugins  
**Range:** 0.0 (silence) to 1.0 (full volume)

```cpp
midi_config.SetVolume(0.7f);   // 70% volume
```

### SetSampleRate(int rate)

**Applies to:** All plugins  
**Common values:** 22050, 44100, 48000  
**Note:** May require plugin reinitialization to take effect

```cpp
midi_config.SetSampleRate(48000);  // High quality
```

### SetPolyphony(int voices)

**Applies to:** FluidSynth, TinySoundFont  
**Purpose:** Maximum simultaneous notes  
**FluidSynth default:** 256  
**TinySoundFont default:** 64

```cpp
midi_config.SetPolyphony(512);  // Allow more notes (higher CPU usage)
```

### SetChipCount(int count)

**Applies to:** OPNMIDI, ADLMIDI  
**Purpose:** Number of emulated sound chips  
**OPNMIDI:** 1 chip = 6 channels, max 100 chips  
**ADLMIDI:** 1 chip = 18 channels, max 100 chips

```cpp
// OPNMIDI: 4 chips = 24 channels
midi_config.SetChipCount(4);

// ADLMIDI: 2 chips = 36 channels
midi_config.SetChipCount(2);
```

### EnableReverb(bool enable)

**Applies to:** FluidSynth only  
**Purpose:** Spatial reverb effect

```cpp
midi_config.EnableReverb(true);   // Spacious sound
midi_config.EnableReverb(false);  // Dry sound
```

### EnableChorus(bool enable)

**Applies to:** FluidSynth only  
**Purpose:** Chorus/ensemble effect

```cpp
midi_config.EnableChorus(true);   // Rich, wide sound
midi_config.EnableChorus(false);  // Clear sound
```

### GetVersion()

**Returns:** Plugin name and version string

```cpp
const char* version = midi_config.GetVersion();
// Returns: "FluidSynth 2.x MIDI Synthesizer"
//      or: "TinySoundFont Header-Only MIDI Synthesizer"
//      or: "OPNMIDI YM2612 Chip Emulator"
//      etc.
```

### GetDefaultBank()

**Returns:** Default soundfont/bank path for the plugin

```cpp
const char* path = midi_config.GetDefaultBank();
// FluidSynth: "/usr/share/sounds/sf2/FluidR3_GM.sf2"
// OPNMIDI: "Embedded Bank #0"
```

## Complete Examples

### Example 1: Game Music Player with FluidSynth

```cpp
#include "src/AudioDecode.h"
#include <hgl/audio/AudioPlayer.h>

void PlayGameMusic(const char* midi_file)
{
    // Configure FluidSynth for best quality
    AudioMidiConfigInterface midi_config;
    if (GetAudioMidiInterface(OS_TEXT("MIDI"), &midi_config))
    {
        midi_config.SetSoundFont("/usr/share/sounds/sf2/GeneralUser_GS.sf2");
        midi_config.SetSampleRate(48000);
        midi_config.SetPolyphony(256);
        midi_config.SetVolume(0.75f);
        midi_config.EnableReverb(true);
        midi_config.EnableChorus(true);
        
        printf("Using: %s\n", midi_config.GetVersion());
    }
    
    // Play the music
    AudioPlayer player;
    player.Load(midi_file);
    player.Play(true);  // Loop
}
```

### Example 2: Retro Game with Genesis Sound (OPNMIDI)

```cpp
void PlayGenesisMusic(const char* midi_file, int bank)
{
    // Configure OPNMIDI for authentic Genesis sound
    AudioMidiConfigInterface midi_config;
    if (GetAudioMidiInterface(OS_TEXT("MIDI"), &midi_config))
    {
        midi_config.SetBankID(bank);        // Sonic 3 bank
        midi_config.SetChipCount(2);        // 2 chips = 12 channels
        midi_config.SetVolume(0.9f);
        midi_config.SetSampleRate(44100);
        
        printf("Genesis mode: %s\n", midi_config.GetVersion());
    }
    
    AudioPlayer player;
    player.Load(midi_file);
    player.Play(true);
}
```

### Example 3: DOS Game Sound (ADLMIDI)

```cpp
void PlayDOSMusic(const char* midi_file)
{
    // Configure ADLMIDI for classic AdLib/Sound Blaster
    AudioMidiConfigInterface midi_config;
    if (GetAudioMidiInterface(OS_TEXT("MIDI"), &midi_config))
    {
        midi_config.SetBankID(0);           // DMX OP2 (Doom sound)
        midi_config.SetChipCount(2);        // 2 OPL3 chips
        midi_config.SetVolume(1.0f);
        
        printf("DOS mode: %s\n", midi_config.GetVersion());
    }
    
    AudioPlayer player;
    player.Load(midi_file);
    player.Play(false);  // Play once
}
```

### Example 4: Zero-Dependency Deployment (TinySoundFont)

```cpp
void PlayPortableMusic(const char* midi_file, const char* sf2_file)
{
    // TinySoundFont: Always available, no external dependencies
    AudioMidiConfigInterface midi_config;
    if (GetAudioMidiInterface(OS_TEXT("MIDI"), &midi_config))
    {
        midi_config.SetSoundFont(sf2_file);  // Bundled soundfont
        midi_config.SetVolume(0.8f);
        
        printf("Using: %s\n", midi_config.GetVersion());
    }
    
    AudioPlayer player;
    player.Load(midi_file);
    player.Play(true);
}
```

## Plugin Feature Matrix

| Feature | FluidSynth | TinySoundFont | WildMIDI | Timidity | OPNMIDI | ADLMIDI |
|---------|-----------|---------------|----------|----------|---------|---------|
| **SetSoundFont** | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ❌ No-op | ❌ No-op |
| **SetBankID** | ❌ No-op | ❌ No-op | ❌ No-op | ❌ No-op | ✅ Yes | ✅ Yes |
| **SetVolume** | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes |
| **SetSampleRate** | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes |
| **SetPolyphony** | ✅ Yes | ✅ Yes | ❌ No-op | ❌ No-op | ❌ No-op | ❌ No-op |
| **SetChipCount** | ❌ No-op | ❌ No-op | ❌ No-op | ❌ No-op | ✅ Yes | ✅ Yes |
| **EnableReverb** | ✅ Yes | ❌ No-op | ❌ No-op | ❌ No-op | ❌ No-op | ❌ No-op |
| **EnableChorus** | ✅ Yes | ❌ No-op | ❌ No-op | ❌ No-op | ❌ No-op | ❌ No-op |
| **GetVersion** | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes |
| **GetDefaultBank** | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes |

## Best Practices

### 1. Configure Before Playing

Always configure MIDI settings **before** loading and playing MIDI files:

```cpp
// ✅ GOOD: Configure first
midi_config.SetSoundFont("/path/to/soundfont.sf2");
player.Load("music.mid");
player.Play(true);

// ❌ BAD: Configure after (may not apply)
player.Load("music.mid");
player.Play(true);
midi_config.SetSoundFont("/path/to/soundfont.sf2");  // Too late!
```

### 2. Check Configuration Success

```cpp
AudioMidiConfigInterface midi_config;
if (!GetAudioMidiInterface(OS_TEXT("MIDI"), &midi_config))
{
    fprintf(stderr, "MIDI plugin not loaded or doesn't support configuration\n");
    return;
}
```

### 3. Use Appropriate Functions for Each Plugin

Check the feature matrix above and only use supported functions:

```cpp
if (strstr(midi_config.GetVersion(), "FluidSynth"))
{
    midi_config.EnableReverb(true);   // OK for FluidSynth
    midi_config.EnableChorus(true);   // OK for FluidSynth
}
else if (strstr(midi_config.GetVersion(), "OPNMIDI"))
{
    midi_config.SetBankID(74);        // OK for OPNMIDI
    midi_config.SetChipCount(4);      // OK for OPNMIDI
}
```

### 4. Handle Missing SoundFonts

```cpp
const char* default_path = midi_config.GetDefaultBank();
if (access(default_path, R_OK) != 0)
{
    fprintf(stderr, "Warning: Default soundfont not found: %s\n", default_path);
    // Try alternative or download
}
```

## Environment Variables

All plugins respect environment variables as fallback:

- **FluidSynth:** `FLUIDSYNTH_SF2`
- **TinySoundFont:** `TSF_SOUNDFONT`
- **WildMIDI:** `WILDMIDI_CFG`
- **Timidity:** `TIMIDITY_CFG`
- **OPNMIDI:** Bank ID via API only
- **ADLMIDI:** Bank ID via API only

Priority: **API call > Environment variable > Default path**

## Troubleshooting

### Problem: Changes don't take effect

**Solution:** Some settings require plugin reinitialization:
- Configure **before** first MIDI load
- Restart application after changing sample rate

### Problem: No sound with custom soundfont

**Solution:** Check file path and permissions:
```bash
ls -l /path/to/soundfont.sf2
# Should show readable file
```

### Problem: Low volume with chip emulators

**Solution:** Increase chip count or volume:
```cpp
midi_config.SetChipCount(4);   // More channels
midi_config.SetVolume(1.0f);   // Max volume
```

## API Integration

### C++ Class Wrapper Example

```cpp
class MIDIPlayer {
private:
    AudioMidiConfigInterface config;
    AudioPlayer player;
    bool config_loaded;
    
public:
    MIDIPlayer() : config_loaded(false) {
        config_loaded = GetAudioMidiInterface(OS_TEXT("MIDI"), &config);
    }
    
    void SetQuality(const char* sf2_path, int sample_rate = 48000) {
        if (config_loaded) {
            config.SetSoundFont(sf2_path);
            config.SetSampleRate(sample_rate);
            config.SetPolyphony(256);
        }
    }
    
    void PlayRetro(const char* midi_file, int bank_id) {
        if (config_loaded) {
            config.SetBankID(bank_id);
            config.SetChipCount(4);
        }
        player.Load(midi_file);
        player.Play(true);
    }
};
```

## See Also

- Plugin README files in `Plug-Ins/Audio.*/README.md`
- AudioPlayer documentation in `AudioPlayer_Analysis.md`
- Quick Reference Guide in `QUICK_REFERENCE.md`
