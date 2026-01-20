# TinySoundFont Plugin for CMAudio

This plugin adds MIDI file playback support to CMAudio using TinySoundFont, a lightweight, header-only SoundFont synthesizer. This is the fourth MIDI plugin option.

## Features

- Decodes and plays MIDI files (.mid, .midi)
- Streaming playback support
- Uses TinySoundFont for software synthesis
- Outputs stereo 16-bit PCM at 44.1kHz
- **No external library dependencies** - header-only implementation
- Lightweight and easy to build
- Cross-platform (Linux, Windows, macOS)

## Why Choose TinySoundFont?

**TinySoundFont Advantages:**
- **No Dependencies**: Header-only library, no external libs needed
- **Easy to Build**: Always compiles, no library detection issues
- **Lightweight**: Small code footprint
- **Cross-Platform**: Works everywhere C++ works
- **Good Quality**: SoundFont-based synthesis
- **Simple Integration**: Just needs a SoundFont file at runtime

**Comparison with Other MIDI Plugins:**

| Feature | TinySoundFont | FluidSynth | WildMIDI | Libtimidity |
|---------|---------------|-----------|----------|-------------|
| Build Dependencies | None | libfluidsynth-dev | libwildmidi-dev | libtimidity-dev |
| Sound Quality | Good | Excellent | Good | Good |
| File Size | Small | Large | Medium | Medium |
| CPU Usage | Low-Medium | Higher | Medium | Low |
| Memory Usage | ~10-30MB | ~20-50MB | ~5-10MB | ~3-5MB |
| Configuration | SoundFont | SoundFont | Timidity cfg | Timidity cfg |

**Choose TinySoundFont for:**
- Easy build process without external dependencies
- Cross-platform compatibility
- When you want a self-contained solution
- Good balance of quality and simplicity

**Choose FluidSynth for:**
- Best possible sound quality
- Professional audio applications
- When external dependencies are not a concern

**Choose WildMIDI or Libtimidity for:**
- Minimal resource usage
- When SoundFont is not available
- Using existing Timidity configuration

## Requirements

### Build Time
**No external libraries needed!** TinySoundFont is header-only and included in the plugin directory.

### Runtime
Only requires a SoundFont (.sf2) file:

**Ubuntu/Debian:**
```bash
sudo apt-get install fluid-soundfont-gm
```

**Fedora/RHEL:**
```bash
sudo dnf install fluid-soundfont-gm
```

**Arch Linux:**
```bash
sudo pacman -s soundfont-fluid
```

## Configuration

### SoundFont Path

The plugin looks for SoundFonts at:
- Environment variable `TSF_SOUNDFONT` (if set)
- `/usr/share/sounds/sf2/FluidR3_GM.sf2` (Linux default)
- `/usr/share/sounds/sf2/default.sf2` (alternative)
- `C:\soundfonts\default.sf2` (Windows default)

Set a custom SoundFont path:
```bash
export TSF_SOUNDFONT=/path/to/your/soundfont.sf2
```

### Quality SoundFonts

Same SoundFonts as FluidSynth:

**Free SoundFonts:**
- **FluidR3_GM** (included with fluid-soundfont-gm package)
- **GeneralUser GS** (high quality, ~30MB)
- **Timbres of Heaven** (very high quality, ~350MB)

## Usage

Once the plugin is built, it works like all other MIDI plugins:

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
- **Sample Rate**: 44100 Hz (fixed)
- **Output**: Always stereo
- **Streaming**: Supports streaming playback via the standard plugin interface
- **Memory**: MIDI files loaded into memory, synthesized on-the-fly
- **Dependencies**: None (header-only)

## Implementation Notes

The plugin follows the standard CMAudio plugin interface:

- `LoadMIDI()`: Synthesizes entire MIDI to PCM (for AudioBuffer)
- `OpenMIDI()`: Opens MIDI for streaming (for AudioPlayer)
- `ReadMIDI()`: Reads synthesized PCM data
- `RestartMIDI()`: Restarts playback from beginning
- `CloseMIDI()`: Closes MIDI stream and frees resources

### TinySoundFont API Usage

- `tsf_load_filename()` for loading SoundFonts
- `tsf_set_output()` for configuring output format
- `tsf_reset()` for resetting synthesizer state
- `tsf_channel_*()` for channel control
- `tsf_render_short()` for rendering audio
- `tml_load_memory()` for loading MIDI data
- `tml_free()` for cleanup

## Building

The plugin is **always** built since it has no external dependencies:

```bash
cd CMAudio
mkdir build
cd build
cmake ..
make
```

The plugin will always compile successfully. You just need a SoundFont file at runtime.

## Plugin Coexistence

All four MIDI plugins (TinySoundFont, FluidSynth, WildMIDI, Timidity) use the same plugin name "MIDI", so only one will be active at runtime.

**Plugin Loading Priority:**
The active plugin depends on build order and which libraries are installed. TinySoundFont will always be built since it has no dependencies.

To prefer TinySoundFont:
1. Build with TinySoundFont (always available)
2. Don't install other MIDI library development packages
3. Or manage plugin loading order in your system

## Advantages Over Other Plugins

### vs FluidSynth
- ✅ No external library dependency
- ✅ Easier to build and deploy
- ✅ Smaller binary size
- ✅ Lower memory usage
- ⚠️ Slightly lower sound quality

### vs WildMIDI/Libtimidity
- ✅ No external library dependency
- ✅ Uses SoundFont (more flexible)
- ✅ Cross-platform without system config
- ⚠️ Slightly higher memory usage

## Troubleshooting

### No sound output
- Check that a SoundFont file exists at the configured path
- Install fluid-soundfont-gm package
- Set TSF_SOUNDFONT environment variable

### Poor sound quality
- Use a higher-quality SoundFont file
- Try GeneralUser GS or Timbres of Heaven

### Plugin fails to initialize
- Verify SoundFont file exists and is readable
- Check SoundFont file is valid (.sf2 format)
- Try a different SoundFont file

### High memory usage
- Use a smaller SoundFont file
- FluidR3_GM (~150MB) is a good balance

### Multiple MIDI plugins installed
- Only one MIDI plugin is active at runtime
- TinySoundFont is always built since it has no dependencies
- Other plugins only build if their libraries are installed

## Performance Characteristics

**Build Time:**
- Fast - no library detection, just compile

**Runtime CPU Usage:**
- Similar to WildMIDI
- Lower than FluidSynth
- Higher than Libtimidity

**Memory Usage:**
- ~10-30MB depending on SoundFont
- Between WildMIDI and FluidSynth

**Sound Quality:**
- Good - SoundFont-based synthesis
- Better than WildMIDI/Libtimidity
- Slightly below FluidSynth

## Advanced Configuration

TinySoundFont is configured through code. To customize:

Edit `MidiRead.cpp` in `InitTinySoundFont()`:

```cpp
// Change sample rate (default: 44100)
tsf_set_output(g_tsf, TSF_STEREO_INTERLEAVED, 48000, 0.0f);

// Adjust global gain
tsf_set_volume(g_tsf, 0.8f);
```

## References

- [TinySoundFont GitHub](https://github.com/schellingb/TinySoundFont)
- [SoundFont Format](https://en.wikipedia.org/wiki/SoundFont)
- [SoundFont Resources](http://www.synthfont.com/links_to_soundfonts.html)

## License

This plugin code follows the CMAudio project license. TinySoundFont library is licensed under MIT license.
