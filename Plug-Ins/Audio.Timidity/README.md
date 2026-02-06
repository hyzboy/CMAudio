# Libtimidity Plugin for CMAudio

This plugin adds MIDI file playback support to CMAudio using the libtimidity library, which is an alternative to the WildMIDI plugin.

## Features

- Decodes and plays MIDI files (.mid, .midi)
- Streaming playback support
- Uses libtimidity for MIDI synthesis
- Outputs stereo 16-bit PCM at 44.1kHz

## Libtimidity vs WildMIDI

Both plugins provide MIDI playback functionality. Choose based on your needs:

**Libtimidity:**
- Simple, lightweight library
- Good for basic MIDI playback
- Smaller dependency footprint

**WildMIDI:**
- More actively maintained
- Better compatibility with various MIDI files
- More advanced features

You can install both plugins and they will both use the same plugin name "MIDI", so only one will be active based on plugin loading order.

## Requirements

### Linux/Unix

Install libtimidity library:

**Ubuntu/Debian:**
```bash
sudo apt-get install libtimidity-dev timidity
```

**Fedora/RHEL:**
```bash
sudo dnf install libtimidity-devel timidity++
```

**Arch Linux:**
```bash
sudo pacman -S libtimidity timidity++
```

### Configuration

Libtimidity requires a Timidity configuration file for instrument patches. The plugin looks for the config at:
- Environment variable `TIMIDITY_CFG` (if set)
- `/etc/timidity/timidity.cfg` (Linux/Unix default)
- `C:\timidity\timidity.cfg` (Windows default)

You can set a custom configuration path:
```bash
export TIMIDITY_CFG=/path/to/your/timidity.cfg
```

### Timidity Configuration

If Timidity is not configured, you may need to install sound fonts:

**Ubuntu/Debian:**
```bash
sudo apt-get install freepats
```

This will install free instrument patches and configure Timidity automatically.

## Usage

Once the plugin is built and installed, it works exactly like the WildMIDI plugin:

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

- **Format**: MIDI files are converted to stereo 16-bit PCM
- **Sample Rate**: 44100 Hz (fixed)
- **Output**: Always stereo
- **Streaming**: Supports streaming playback via the standard plugin interface
- **Memory**: MIDI files are loaded into memory, then decoded on-the-fly

## Implementation Notes

The plugin follows the standard CMAudio plugin interface:

- `LoadMIDI()`: Decodes entire MIDI to PCM (for AudioBuffer)
- `OpenMIDI()`: Opens MIDI for streaming (for AudioPlayer)
- `ReadMIDI()`: Reads PCM data from MIDI stream
- `RestartMIDI()`: Restarts playback from beginning
- `CloseMIDI()`: Closes MIDI stream and frees resources

## API Differences from WildMIDI

Libtimidity uses a different API:
- `mid_init()` / `mid_exit()` for library initialization
- `mid_song_load_dls()` for loading MIDI data
- `mid_song_read_wave()` for reading PCM output
- `mid_song_free()` for cleanup

## Building

The plugin is automatically included in the CMAudio build if libtimidity is detected:

```bash
cd CMAudio
mkdir build
cd build
cmake ..
make
```

If libtimidity is not found, the plugin will be skipped with an informational message.

## Troubleshooting

### "libtimidity not found" error
- Install libtimidity-dev package
- Ensure libtimidity library is in standard library paths

### No sound or incorrect instruments
- Install instrument patches (freepats or timidity-freepats)
- Check that `/etc/timidity/timidity.cfg` exists and is properly configured
- Verify the config path in the plugin code matches your system

### Plugin fails to initialize
- Check that Timidity configuration file is readable
- Ensure instrument patches are installed
- Verify libtimidity library version compatibility

### Both WildMIDI and Timidity plugins installed
- Only one MIDI plugin will be active at a time
- Plugin loading order determines which one is used
- Both plugins share the same plugin name "MIDI"

## Performance Comparison

**Libtimidity:**
- Lower memory usage
- Faster initialization
- Good for simple MIDI files

**WildMIDI:**
- Better sound quality on complex MIDI files
- More accurate MIDI rendering
- Better maintained codebase

## References

- [Libtimidity](http://libtimidity.sourceforge.net/)
- [Timidity++](http://timidity.sourceforge.net/)
- [FreePats Project](https://freepats.zenvoid.org/)

## License

This plugin code follows the CMAudio project license. Libtimidity library is licensed under LGPL.
