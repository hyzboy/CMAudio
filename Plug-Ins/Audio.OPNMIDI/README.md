# OPNMIDI Plugin for CMAudio

This plugin adds MIDI file playback support to CMAudio using OPNMIDI, which provides authentic Yamaha YM2612/OPN2 chip emulation. This is the same chip used in the Sega Genesis/Mega Drive console.

## Features

- Decodes and plays MIDI files (.mid, .midi)
- Streaming playback support
- Uses OPNMIDI for authentic OPN2/YM2612 chip emulation
- Outputs stereo 16-bit PCM at 44.1kHz
- Authentic Sega Genesis/Mega Drive sound
- Built-in WOPN (Yamaha OPN2) instrument banks
- 4-chip mode for higher polyphony (64 channels)

## Why Choose OPNMIDI?

**OPNMIDI Advantages:**
- **Authentic Retro Sound**: Perfect emulation of Sega Genesis/Mega Drive
- **Nostalgic**: Classic 16-bit console sound
- **Built-in Banks**: No external files needed
- **Efficient**: Low CPU usage
- **Unique Character**: Distinctive FM synthesis sound

**Use Cases:**
- Retro game music
- Sega Genesis/Mega Drive style soundtracks
- Chiptune music
- When you want that authentic 16-bit console sound
- Nostalgia-focused applications

## Requirements

### Linux/Unix

Install libOPNMIDI library:

**Ubuntu/Debian:**
```bash
sudo apt-get install libopnmidi-dev
```

**From Source:**
```bash
git clone https://github.com/Wohlstand/libADLMIDI
cd libADLMIDI
mkdir build && cd build
cmake -DWITH_OPNMIDI=ON ..
make
sudo make install
```

### Configuration

OPNMIDI uses built-in WOPN instrument banks - no external configuration needed!

The plugin automatically:
- Uses 4-chip mode (64 channels total)
- Loads default WOPN bank
- Configures for optimal quality

## Usage

Once the plugin is built, it works like all other MIDI plugins:

```cpp
// Load and play a MIDI file with Genesis sound
AudioPlayer midi_player;
midi_player.Load("sonic.mid");
midi_player.Play(true);  // Loop playback

// Or use AudioBuffer for one-shot playback
AudioBuffer midi_buffer("streets_of_rage.midi");
AudioSource source;
source.Link(&midi_buffer);
source.Play();
```

## Technical Details

- **Format**: MIDI files synthesized via YM2612/OPN2 emulation
- **Sample Rate**: 44100 Hz (fixed)
- **Output**: Always stereo
- **Chips**: 4 virtual YM2612 chips (64 channels total)
- **Streaming**: Supports streaming playback via the standard plugin interface
- **Memory**: MIDI files loaded into memory, synthesized on-the-fly

## Implementation Notes

The plugin follows the standard CMAudio plugin interface:

- `LoadMIDI()`: Synthesizes entire MIDI to PCM (for AudioBuffer)
- `OpenMIDI()`: Opens MIDI for streaming (for AudioPlayer)
- `ReadMIDI()`: Reads synthesized PCM data
- `RestartMIDI()`: Restarts playback from beginning
- `CloseMIDI()`: Closes MIDI stream and frees resources

### OPNMIDI API Usage

- `opn2_init()` / `opn2_close()` for device management
- `opn2_setNumChips()` for multi-chip mode
- `opn2_openBankData()` for loading banks
- `opn2_openData()` for loading MIDI
- `opn2_playFormat()` for rendering audio
- `opn2_positionRewind()` for restart

## Building

The plugin is automatically included in the CMAudio build if libOPNMIDI is detected:

```bash
cd CMAudio
mkdir build
cd build
cmake ..
make
```

If libOPNMIDI is not found, the plugin will be skipped with an informational message.

## Plugin Coexistence

All MIDI plugins use the same plugin name "MIDI", so only one will be active at runtime.

OPNMIDI provides a unique retro sound that sets it apart from other MIDI plugins.

## Comparison with Other Plugins

| Feature | OPNMIDI | ADLMIDI | FluidSynth | TinySoundFont |
|---------|---------|---------|-----------|---------------|
| **Sound Style** | Genesis/MD | DOS/AdLib | Modern/Real | Modern/Real |
| **Chip Type** | YM2612/OPN2 | OPL3 | SoundFont | SoundFont |
| **Polyphony** | 64 channels | 36 channels | 256+ | Good |
| **CPU Usage** | Low | Low | Higher | Low-Medium |
| **Best For** | Retro games | DOS games | Professional | General |

**Choose OPNMIDI for:**
- Authentic Sega Genesis/Mega Drive sound
- Retro/chiptune music
- 16-bit console nostalgia
- When you want FM synthesis character

**Choose ADLMIDI for:**
- Authentic DOS game sound
- AdLib/SoundBlaster nostalgia
- Classic PC gaming sound

**Choose FluidSynth/TinySoundFont for:**
- Modern, realistic instrument sounds
- When you want orchestral quality

## Performance

**CPU Usage:** Very low (~1-2%)
**Memory Usage:** ~5-10MB
**Init Time:** Fast (~10-20ms)
**Sound Quality:** Authentic retro FM synthesis

## Troubleshooting

### "libOPNMIDI not found" error
- Install libopnmidi-dev package
- Or build from source (see Requirements)

### Sound is too "retro" or "chippy"
- This is intentional! OPNMIDI provides authentic Genesis sound
- For modern sound, use FluidSynth or TinySoundFont instead

### Low polyphony
- OPNMIDI uses 4 chips by default (64 channels)
- This is higher than original Genesis (1 chip = 16 channels)
- Complex MIDIs may still exceed capacity

### Multiple MIDI plugins installed
- Only one MIDI plugin is active at runtime
- Plugin loading order determines which is used

## Authentic Genesis/Mega Drive Sound

OPNMIDI accurately emulates the YM2612 chip:
- Same chip used in Sega Genesis, Mega Drive
- FM synthesis (not sample-based)
- Distinctive "metallic" sound
- Perfect for:
  - Sonic the Hedgehog style music
  - Streets of Rage soundtracks
  - Any 16-bit Sega game music

## Advanced Configuration

Edit `MidiRead.cpp` to customize:

```cpp
// Change number of chips (more chips = more polyphony)
opn2_setNumChips(device, 6);  // 6 chips = 96 channels

// Use different bank (if you have custom WOPN banks)
opn2_openBankFile(device, "custom.wopn");
```

## References

- [libADLMIDI GitHub](https://github.com/Wohlstand/libADLMIDI)
- [YM2612 Info](https://en.wikipedia.org/wiki/Yamaha_YM2612)
- [Sega Genesis Sound](https://segaretro.org/Sega_Mega_Drive/Technical_specifications#Audio)

## License

This plugin code follows the CMAudio project license. libOPNMIDI/libADLMIDI library is licensed under LGPL v3.
