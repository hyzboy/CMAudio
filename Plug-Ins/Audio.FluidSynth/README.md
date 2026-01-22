# FluidSynth Plugin for CMAudio

This plugin adds MIDI file playback support to CMAudio using FluidSynth, a high-quality software synthesizer. This is the third MIDI plugin option alongside WildMIDI and Libtimidity.

## Features

- Decodes and plays MIDI files (.mid, .midi)
- Streaming playback support
- Uses FluidSynth for high-quality software synthesis
- Outputs stereo 16-bit PCM at 44.1kHz
- Built-in reverb and chorus effects
- High polyphony (256 voices by default)

## Why Choose FluidSynth?

**FluidSynth Advantages:**
- **Best Sound Quality**: Professional-grade synthesis with SoundFont support
- **Rich Features**: Built-in reverb, chorus, and other effects
- **Widely Used**: Industry-standard software synthesizer
- **Active Development**: Well-maintained and regularly updated
- **Flexible**: Highly configurable synthesis parameters

**Comparison with Other MIDI Plugins:**

| Feature | FluidSynth | WildMIDI | Libtimidity |
|---------|-----------|----------|-------------|
| Sound Quality | Excellent | Good | Good |
| Polyphony | 256+ voices | Limited | Limited |
| Effects | Built-in | None | None |
| CPU Usage | Higher | Medium | Low |
| Memory Usage | Higher | Medium | Low |
| Configuration | SoundFont | Timidity cfg | Timidity cfg |

**Choose FluidSynth for:**
- Professional-quality MIDI playback
- Complex multi-instrument MIDI files
- When sound quality is critical
- When you have good SoundFont files available

**Choose WildMIDI or Libtimidity for:**
- Simple MIDI playback needs
- Resource-constrained environments
- Faster initialization

## Requirements

### Linux/Unix

Install FluidSynth library and SoundFonts:

**Ubuntu/Debian:**
```bash
sudo apt-get install libfluidsynth-dev fluidsynth fluid-soundfont-gm
```

**Fedora/RHEL:**
```bash
sudo dnf install fluidsynth-devel fluidsynth fluid-soundfont-gm
```

**Arch Linux:**
```bash
sudo pacman -S fluidsynth soundfont-fluid
```

### Configuration

#### SoundFont Path

FluidSynth requires a SoundFont file (.sf2) for synthesis. The plugin looks for SoundFonts at:
- Environment variable `FLUIDSYNTH_SF2` (if set)
- `/usr/share/sounds/sf2/FluidR3_GM.sf2` (Linux default)
- `/usr/share/sounds/sf2/default.sf2` (alternative)
- `C:\soundfonts\default.sf2` (Windows default)

Set a custom SoundFont path:
```bash
export FLUIDSYNTH_SF2=/path/to/your/soundfont.sf2
```

#### Quality SoundFonts

For best results, use high-quality SoundFont files:

**Free SoundFonts:**
- **FluidR3_GM** (included with fluid-soundfont-gm package)
- **GeneralUser GS** (high quality, ~30MB)
- **Timbres of Heaven** (very high quality, ~350MB)

**Download SoundFonts:**
```bash
# Install default SoundFont
sudo apt-get install fluid-soundfont-gm

# Or download GeneralUser GS
wget http://www.schristiancollins.com/generaluser/GeneralUser_GS_1.471.zip
unzip GeneralUser_GS_1.471.zip
export FLUIDSYNTH_SF2=/path/to/GeneralUser_GS.sf2
```

## Usage

Once the plugin is built and installed, it works like the other MIDI plugins:

```cpp
// Load and play a MIDI file
AudioPlayer midi_player;
midi_player.Load("music.mid");
midi_player.Play(true);  // Loop playback

// Or use AudioBuffer for one-shot playback
AudioBuffer midi_buffer("sound.midi");
AudioSource source;
source.Link(&midi_buffer);
source.Play();
```

## Technical Details

- **Format**: MIDI files synthesized to stereo 16-bit PCM
- **Sample Rate**: 44100 Hz (configurable in code)
- **Output**: Always stereo
- **Polyphony**: 256 voices (configurable in code)
- **Effects**: Reverb and chorus enabled by default
- **Streaming**: Supports streaming playback via the standard plugin interface
- **Memory**: MIDI files loaded into memory, synthesized on-the-fly

## Implementation Notes

The plugin follows the standard CMAudio plugin interface:

- `LoadMIDI()`: Synthesizes entire MIDI to PCM (for AudioBuffer)
- `OpenMIDI()`: Opens MIDI for streaming (for AudioPlayer)
- `ReadMIDI()`: Reads synthesized PCM data
- `RestartMIDI()`: Restarts playback from beginning
- `CloseMIDI()`: Closes MIDI stream and frees resources

### FluidSynth API Usage

- `new_fluid_settings()` / `delete_fluid_settings()` for configuration
- `new_fluid_synth()` / `delete_fluid_synth()` for synthesizer
- `fluid_synth_sfload()` for loading SoundFonts
- `new_fluid_player()` / `delete_fluid_player()` for MIDI playback
- `fluid_synth_write_s16()` for rendering audio

## Building

The plugin is automatically included in the CMAudio build if FluidSynth is detected:

```bash
cd CMAudio
mkdir build
cd build
cmake ..
make
```

If FluidSynth is not found, the plugin will be skipped with an informational message.

## Plugin Coexistence

All three MIDI plugins (FluidSynth, WildMIDI, Timidity) use the same plugin name "MIDI", so only one will be active at runtime. The active plugin is determined by plugin loading order.

To use a specific plugin:
1. Install only the library you want (libfluidsynth-dev, libwildmidi-dev, or libtimidity-dev)
2. Or manage plugin loading order in your system

## Performance Tuning

### Adjusting Polyphony

Edit `MidiRead.cpp` to change polyphony:
```cpp
fluid_settings_setint(fluid_settings, "synth.polyphony", 128);  // Lower for less CPU
```

### Disabling Effects

To reduce CPU usage, disable effects:
```cpp
fluid_settings_setint(fluid_settings, "synth.reverb.active", 0);
fluid_settings_setint(fluid_settings, "synth.chorus.active", 0);
```

### Sample Rate

Change sample rate (must match OpenAL):
```cpp
fluid_settings_setnum(fluid_settings, "synth.sample-rate", 48000.0);
```

## Troubleshooting

### "FluidSynth not found" error
- Install libfluidsynth-dev package
- Ensure FluidSynth library is in standard library paths

### No sound output
- Check that a SoundFont file is loaded successfully
- Verify SoundFont path is correct
- Install fluid-soundfont-gm package
- Set FLUIDSYNTH_SF2 environment variable

### Poor sound quality
- Use a higher-quality SoundFont file
- Check that reverb and chorus are enabled
- Increase polyphony if notes are being cut off

### High CPU usage
- Reduce polyphony setting
- Disable reverb and chorus effects
- Use a simpler SoundFont file

### Plugin initialization fails
- Check that SoundFont file exists and is readable
- Verify FluidSynth library version (2.x recommended)
- Check error messages in application logs

### Multiple MIDI plugins installed
- Only one MIDI plugin is active at runtime
- Plugin loading order determines which is used
- All plugins share the same name "MIDI"

## Performance Comparison

**CPU Usage (typical MIDI file):**
- FluidSynth: ~5-10% (with effects)
- WildMIDI: ~2-3%
- Libtimidity: ~1-2%

**Memory Usage:**
- FluidSynth: ~20-50MB (depends on SoundFont)
- WildMIDI: ~5-10MB
- Libtimidity: ~3-5MB

**Initialization Time:**
- FluidSynth: ~100-500ms (loading SoundFont)
- WildMIDI: ~10-50ms
- Libtimidity: ~10-50ms

**Sound Quality:**
- FluidSynth: Excellent (best of the three)
- WildMIDI: Good
- Libtimidity: Good

## Advanced Configuration

### Custom FluidSynth Settings

Modify `InitFluidSynth()` in MidiRead.cpp for custom settings:

```cpp
// Increase polyphony for complex files
fluid_settings_setint(fluid_settings, "synth.polyphony", 512);

// Adjust reverb settings
fluid_settings_setnum(fluid_settings, "synth.reverb.room-size", 0.5);
fluid_settings_setnum(fluid_settings, "synth.reverb.damp", 0.4);

// Adjust chorus settings
fluid_settings_setnum(fluid_settings, "synth.chorus.depth", 2.0);
fluid_settings_setnum(fluid_settings, "synth.chorus.level", 1.5);
```

## References

- [FluidSynth Official Site](https://www.fluidsynth.org/)
- [FluidSynth Documentation](https://www.fluidsynth.org/api/)
- [SoundFont Resources](http://www.synthfont.com/links_to_soundfonts.html)
- [GeneralUser GS SoundFont](http://www.schristiancollins.com/generaluser.php)

## License

This plugin code follows the CMAudio project license. FluidSynth library is licensed under LGPL v2.1.
