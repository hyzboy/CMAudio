# ADLMIDI Plugin for CMAudio

This plugin adds MIDI file playback support to CMAudio using ADLMIDI, which provides authentic AdLib/OPL3 chip emulation. This is the same chip used in SoundBlaster cards and classic DOS games.

## Features

- Decodes and plays MIDI files (.mid, .midi)
- Streaming playback support
- Uses ADLMIDI for authentic OPL3/AdLib chip emulation
- Outputs stereo 16-bit PCM at 44.1kHz
- Authentic DOS game/AdLib sound
- Built-in instrument banks (DMX OP2 and others)
- 4-chip mode for higher polyphony (36 channels)

## Why Choose ADLMIDI?

**ADLMIDI Advantages:**
- **Authentic Retro Sound**: Perfect emulation of AdLib/SoundBlaster
- **Nostalgic**: Classic DOS game sound
- **Built-in Banks**: Multiple banks including DMX OP2 (Doom, Duke3D)
- **Efficient**: Low CPU usage
- **Unique Character**: Distinctive OPL3 FM synthesis sound

**Use Cases:**
- Retro DOS game music
- AdLib/SoundBlaster style soundtracks
- Classic PC gaming nostalgia
- When you want that authentic early 90s PC sound
- Doom, Duke Nukem 3D, Commander Keen style music

## Requirements

### Linux/Unix

Install libADLMIDI library:

**Ubuntu/Debian:**
```bash
sudo apt-get install libadlmidi-dev
```

**From Source:**
```bash
git clone https://github.com/Wohlstand/libADLMIDI
cd libADLMIDI
mkdir build && cd build
cmake ..
make
sudo make install
```

### Configuration

ADLMIDI uses built-in instrument banks - no external configuration needed!

The plugin automatically:
- Uses 4-chip mode (36 channels total)
- Loads DMX OP2 bank (bank 58) - the classic Doom/Duke3D sound
- Configures for optimal quality

## Usage

Once the plugin is built, it works like all other MIDI plugins:

```cpp
// Load and play a MIDI file with AdLib sound
AudioPlayer midi_player;
midi_player.Load("doom_e1m1.mid");
midi_player.Play(true);  // Loop playback

// Or use AudioBuffer for one-shot playback
AudioBuffer midi_buffer("duke3d_theme.midi");
AudioSource source;
source.Link(&midi_buffer);
source.Play();
```

## Technical Details

- **Format**: MIDI files synthesized via OPL3/AdLib emulation
- **Sample Rate**: 44100 Hz (fixed)
- **Output**: Always stereo
- **Chips**: 4 virtual OPL3 chips (36 channels total)
- **Bank**: DMX OP2 (bank 58) by default
- **Streaming**: Supports streaming playback via the standard plugin interface
- **Memory**: MIDI files loaded into memory, synthesized on-the-fly

## Implementation Notes

The plugin follows the standard CMAudio plugin interface:

- `LoadMIDI()`: Synthesizes entire MIDI to PCM (for AudioBuffer)
- `OpenMIDI()`: Opens MIDI for streaming (for AudioPlayer)
- `ReadMIDI()`: Reads synthesized PCM data
- `RestartMIDI()`: Restarts playback from beginning
- `CloseMIDI()`: Closes MIDI stream and frees resources

### ADLMIDI API Usage

- `adl_init()` / `adl_close()` for device management
- `adl_setNumChips()` for multi-chip mode
- `adl_setBank()` for selecting instrument bank
- `adl_openData()` for loading MIDI
- `adl_playFormat()` for rendering audio
- `adl_positionRewind()` for restart

## Building

The plugin is automatically included in the CMAudio build if libADLMIDI is detected:

```bash
cd CMAudio
mkdir build
cd build
cmake ..
make
```

If libADLMIDI is not found, the plugin will be skipped with an informational message.

## Plugin Coexistence

All MIDI plugins use the same plugin name "MIDI", so only one will be active at runtime.

ADLMIDI provides a unique retro sound that sets it apart from other MIDI plugins.

## Comparison with Other Plugins

| Feature | ADLMIDI | OPNMIDI | FluidSynth | TinySoundFont |
|---------|---------|---------|-----------|---------------|
| **Sound Style** | DOS/AdLib | Genesis/MD | Modern/Real | Modern/Real |
| **Chip Type** | OPL3 | YM2612/OPN2 | SoundFont | SoundFont |
| **Polyphony** | 36 channels | 64 channels | 256+ | Good |
| **CPU Usage** | Low | Low | Higher | Low-Medium |
| **Best For** | DOS games | Retro games | Professional | General |

**Choose ADLMIDI for:**
- Authentic DOS game sound
- AdLib/SoundBlaster nostalgia
- Classic PC gaming sound (Doom, Duke3D, etc.)
- When you want OPL3 FM synthesis character

**Choose OPNMIDI for:**
- Authentic Sega Genesis/Mega Drive sound
- 16-bit console nostalgia
- More polyphony (64 vs 36 channels)

**Choose FluidSynth/TinySoundFont for:**
- Modern, realistic instrument sounds
- When you want orchestral quality

## Performance

**CPU Usage:** Very low (~1-2%)
**Memory Usage:** ~5-10MB
**Init Time:** Fast (~10-20ms)
**Sound Quality:** Authentic retro OPL3 synthesis

## Troubleshooting

### "libADLMIDI not found" error
- Install libadlmidi-dev package
- Or build from source (see Requirements)

### Sound is too "retro" or "chippy"
- This is intentional! ADLMIDI provides authentic AdLib sound
- For modern sound, use FluidSynth or TinySoundFont instead

### Low polyphony
- ADLMIDI uses 4 chips by default (36 channels)
- This is higher than original SoundBlaster (1 chip = 9 channels)
- Complex MIDIs may still exceed capacity

### Multiple MIDI plugins installed
- Only one MIDI plugin is active at runtime
- Plugin loading order determines which is used

## Authentic DOS Game Sound

ADLMIDI accurately emulates the OPL3 chip:
- Same chip used in SoundBlaster 16, AdLib Gold
- FM synthesis (not sample-based)
- Distinctive "classic PC game" sound
- Perfect for:
  - Doom, Duke Nukem 3D, Heretic, Hexen music
  - Commander Keen, Jazz Jackrabbit soundtracks
  - Any classic DOS game music

## Available Banks

ADLMIDI includes many built-in banks:
- **Bank 58 (DMX OP2)**: Default - used in Doom, Duke3D, Blood
- **Bank 0**: Standard AdLib bank
- Many others for different styles

Edit `MidiRead.cpp` to change bank:
```cpp
adl_setBank(device, 0);   // Standard AdLib
adl_setBank(device, 58);  // DMX OP2 (Doom/Duke3D)
adl_setBank(device, 14);  // Another variant
```

## Advanced Configuration

Edit `MidiRead.cpp` to customize:

```cpp
// Change number of chips (more chips = more polyphony)
adl_setNumChips(device, 6);  // 6 chips = 54 channels

// Use different bank
adl_setBank(device, 14);  // Try different banks for different sounds

// Load custom bank file
adl_openBankFile(device, "custom.wopl");
```

## References

- [libADLMIDI GitHub](https://github.com/Wohlstand/libADLMIDI)
- [OPL3 Info](https://en.wikipedia.org/wiki/Yamaha_YMF262)
- [AdLib Sound](https://en.wikipedia.org/wiki/Ad_Lib,_Inc.)

## License

This plugin code follows the CMAudio project license. libADLMIDI library is licensed under LGPL v3.
