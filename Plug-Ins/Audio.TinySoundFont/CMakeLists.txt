# TinySoundFont is a header-only library included in this directory
# No external dependencies needed - just check if SoundFont files are available

MESSAGE(STATUS "Building TinySoundFont MIDI plugin (header-only library)")
MESSAGE(STATUS "  TinySoundFont is embedded - no external library needed")
MESSAGE(STATUS "  Requires SoundFont (.sf2) file at runtime")
MESSAGE(STATUS "  Set TSF_SOUNDFONT environment variable to specify SoundFont path")

# Include current directory for tsf.h and tml.h headers
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# Use the same plugin name as other MIDI plugins so only one is active
add_cm_plugin(CMP.Audio.MIDI "Plug-Ins//Audio" MidiRead.cpp)

MESSAGE(STATUS "  Default SoundFont paths:")
MESSAGE(STATUS "    Linux: /usr/share/sounds/sf2/FluidR3_GM.sf2")
MESSAGE(STATUS "    Windows: C:\\soundfonts\\default.sf2")
MESSAGE(STATUS "  Install SoundFont: sudo apt-get install fluid-soundfont-gm")
