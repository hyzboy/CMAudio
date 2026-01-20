if(win32)
    set(LIB_3RD_FIND_HINT "C:/Program Files/FluidSynth/lib" "C:/Program Files (x86)/FluidSynth/lib")

    find_package(FluidSynth CONFIG REQUIRED)
    set(AUDIO_FLUIDSYNTH_LIBRARY FluidSynth::libfluidsynth)
else()
    # Try to find system FluidSynth library
    FIND_PATH(AUDIO_FLUIDSYNTH_INCLUDE_DIR
        NAMES fluidsynth.h
        HINTS
        /usr/include
        /usr/local/include
        /usr/include/fluidsynth)

    FIND_LIBRARY(AUDIO_FLUIDSYNTH_LIBRARY
        NAMES fluidsynth
        HINTS
        /usr/lib
        /usr/local/lib
        /usr/lib/x86_64-linux-gnu
        ${LIB_3RD_FIND_HINT})
endif()

IF(AUDIO_FLUIDSYNTH_INCLUDE_DIR AND AUDIO_FLUIDSYNTH_LIBRARY)
    MESSAGE(STATUS "Found FluidSynth: ${AUDIO_FLUIDSYNTH_LIBRARY}")
    INCLUDE_DIRECTORIES(${AUDIO_FLUIDSYNTH_INCLUDE_DIR})

    # Use the same plugin name as WildMIDI and Timidity so only one MIDI plugin is active
    add_cm_plugin(CMP.Audio.FluidSynth "Plug-Ins//Audio" MidiRead.cpp ${AUDIO_FLUIDSYNTH_LIBRARY})
ELSE()
    MESSAGE(STATUS "FluidSynth not found. FluidSynth MIDI plugin will not be built.")
    MESSAGE(STATUS "  To enable FluidSynth MIDI support, install FluidSynth:")
    MESSAGE(STATUS "  Ubuntu/Debian: sudo apt-get install libfluidsynth-dev")
    MESSAGE(STATUS "  Fedora/RHEL:   sudo dnf install fluidsynth-devel")
    MESSAGE(STATUS "  Note: This is an alternative to WildMIDI and Timidity plugins")
ENDIF()
