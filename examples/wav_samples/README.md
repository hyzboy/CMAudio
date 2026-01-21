# WAV Sample Files Required

This directory should contain the following WAV files for the AudioMixer and AudioScene tests.

## Format Requirements

All files must be:
- **Format**: Mono (single channel)
- **Bit depth**: 16-bit PCM
- **Sample rate**: 22050 Hz or 44100 Hz
- **Duration**: 1-5 seconds (looping sounds should be seamless)

## Required Files

### Vehicle Sounds (Looping)

1. **car_small.wav**
   - Small car/sedan engine sound
   - Should loop seamlessly
   - Recommended: Idle or constant speed engine noise
   - Duration: 2-3 seconds

2. **car_suv.wav**
   - SUV/larger vehicle engine sound
   - Should loop seamlessly  
   - Slightly deeper/heavier than small car
   - Duration: 2-3 seconds

3. **car_truck.wav**
   - Large truck engine sound
   - Should loop seamlessly
   - Much deeper/heavier rumble
   - Duration: 2-3 seconds

### Sound Effects (One-shot or Short Loop)

4. **horn_short.wav**
   - Short car horn beep
   - Duration: 0.3-0.5 seconds

5. **horn_long.wav**
   - Long car horn sound
   - Duration: 1-2 seconds

6. **brake_screech.wav**
   - Tire screeching/braking sound
   - Duration: 0.5-1.5 seconds

### Nature Sounds (Looping)

7. **bird_chirp.wav**
   - Bird chirping sound
   - Should loop seamlessly
   - Can be single bird or multiple
   - Duration: 1-3 seconds

8. **bee_buzz.wav**
   - Single bee buzzing sound
   - Should loop seamlessly
   - Will be used to create swarm effect
   - Duration: 1-2 seconds

## Where to Find Samples

You can obtain free mono WAV samples from:

- **Freesound.org**: https://freesound.org/ (CC-licensed sounds)
- **ZapSplat**: https://www.zapsplat.com/ (Free sound effects)
- **SoundBible**: https://soundbible.com/ (Free sound clips)
- **BBC Sound Effects**: https://sound-effects.bbcrewind.co.uk/ (Free BBC archive)

## Converting Samples

If you have stereo or different format files, use FFmpeg to convert:

```bash
# Convert stereo to mono, resample to 22050 Hz, 16-bit PCM
ffmpeg -i input.wav -ac 1 -ar 22050 -sample_fmt s16 -y output.wav

# Or resample to 44100 Hz
ffmpeg -i input.wav -ac 1 -ar 44100 -sample_fmt s16 -y output.wav
```

## File Organization

Place all files directly in this directory:
```
wav_samples/
├── car_small.wav
├── car_suv.wav
├── car_truck.wav
├── horn_short.wav
├── horn_long.wav
├── brake_screech.wav
├── bird_chirp.wav
└── bee_buzz.wav
```

## Testing

Once you have all files in place, run the test programs from the build directory:

```bash
# Basic mixer test (uses car_small.wav)
./mixer_basic_test

# City scene test (uses all vehicle and environment sounds)
./scene_city_test

# Swarm test (uses bee_buzz.wav)
./scene_swarm_test
```

## Notes

- All samples should be **seamlessly looping** where marked, to avoid clicks/pops
- Keep file sizes reasonable (< 1MB each)
- Mono format is required - the mixing system only supports mono audio
- Higher quality sources will produce better mixed results
