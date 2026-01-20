# Try to find system libOPNMIDI library
FIND_PATH(AUDIO_OPNMIDI_INCLUDE_DIR
    NAMES opnmidi.h
    HINTS
    /usr/include
    /usr/local/include)

FIND_LIBRARY(AUDIO_OPNMIDI_LIBRARY
    NAMES OPNMIDI opnmidi
    HINTS
    /usr/lib
    /usr/local/lib
    /usr/lib/x86_64-linux-gnu
    ${LIB_3RD_FIND_HINT})

IF(AUDIO_OPNMIDI_INCLUDE_DIR AND AUDIO_OPNMIDI_LIBRARY)
    MESSAGE(STATUS "Found libOPNMIDI: ${AUDIO_OPNMIDI_LIBRARY}")
    INCLUDE_DIRECTORIES(${AUDIO_OPNMIDI_INCLUDE_DIR})
    
    # Use the same plugin name as other MIDI plugins so only one is active
    add_cm_plugin(CMP.Audio.OPNMIDI "Plug-Ins//Audio" MidiRead.cpp ${AUDIO_OPNMIDI_LIBRARY})
ELSE()
    MESSAGE(STATUS "libOPNMIDI not found. OPNMIDI plugin will not be built.")
    MESSAGE(STATUS "  To enable OPNMIDI (YM2612/OPN chip emulation) support:")
    MESSAGE(STATUS "  Ubuntu/Debian: sudo apt-get install libopnmidi-dev")
    MESSAGE(STATUS "  Or build from: https://github.com/Wohlstand/libADLMIDI")
    MESSAGE(STATUS "  Note: This provides authentic Sega Genesis/Mega Drive sound")
ENDIF()
