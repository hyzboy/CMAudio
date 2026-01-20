# WildMIDI Plugin for CMAudio

This plugin adds MIDI file playback support to CMAudio using the WildMIDI library.

## Features

- Decodes and plays MIDI files (.mid, .midi)
- Streaming playback support
- Uses WildMIDI for high-quality MIDI synthesis
- Outputs stereo 16-bit PCM at 44.1kHz

## Requirements

### Linux/Unix

Install WildMIDI library:

**Ubuntu/Debian:**
```bash
sudo apt-get install libwildmidi-dev timidity
```

**Fedora/RHEL:**
```bash
sudo dnf install wildmidi-devel timidity++
```

**Arch Linux:**
```bash
sudo pacman -S wildmidi timidity++
```

### Configuration

WildMIDI requires a Timidity configuration file for instrument patches. The plugin looks for the config at:
- Environment variable `WILDMIDI_CFG` (if set)
- `/etc/timidity/timidity.cfg` (Linux/Unix default)
- `C:\timidity\timidity.cfg` (Windows default)

You can set a custom configuration path:
```bash
export WILDMIDI_CFG=/path/to/your/timidity.cfg
```

### Timidity Configuration

If Timidity is not configured, you may need to install sound fonts:

**Ubuntu/Debian:**
```bash
sudo apt-get install freepats
```

This will install free instrument patches and configure Timidity automatically.

## Usage

Once the plugin is built and installed, you can use it like any other audio format:

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
- **Output**: Always stereo, even for mono MIDI files
- **Streaming**: Supports streaming playback via the standard plugin interface
- **Memory**: MIDI files are loaded into memory, then decoded on-the-fly

## Implementation Notes

The plugin follows the standard CMAudio plugin interface:

- `LoadMIDI()`: Decodes entire MIDI to PCM (for AudioBuffer)
- `OpenMIDI()`: Opens MIDI for streaming (for AudioPlayer)
- `ReadMIDI()`: Reads PCM data from MIDI stream
- `RestartMIDI()`: Restarts playback from beginning
- `CloseMIDI()`: Closes MIDI stream and frees resources

## Limitations

1. **Windows Support**: Windows build configuration is not yet complete. Contributions welcome!
2. **Sample Rate**: Fixed at 44.1kHz (WildMIDI default)
3. **Channels**: Always outputs stereo
4. **Config Required**: Requires Timidity configuration file with instrument patches

## Building

The plugin is automatically included in the CMAudio build if WildMIDI is detected:

```bash
cd CMAudio
mkdir build
cd build
cmake ..
make
```

If WildMIDI is not found, the plugin will be skipped with an informational message.

## Troubleshooting

### "WildMIDI not found" error
- Install libwildmidi-dev package
- Ensure WildMIDI library is in standard library paths

### No sound or incorrect instruments
- Install instrument patches (freepats or timidity-freepats)
- Check that `/etc/timidity/timidity.cfg` exists and is properly configured
- Verify the config path in the plugin code matches your system

### Plugin fails to initialize
- Check that Timidity configuration file is readable
- Ensure instrument patches are installed
- Verify WildMIDI library version (0.4.x recommended)

## References

- [WildMIDI Project](https://www.mindwerks.net/projects/wildmidi/)
- [Timidity++](http://timidity.sourceforge.net/)
- [FreePats Project](https://freepats.zenvoid.org/)

## License

This plugin code follows the CMAudio project license. WildMIDI library is licensed under LGPL v3.
