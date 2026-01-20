# Try to find system libADLMIDI library
FIND_PATH(AUDIO_ADLMIDI_INCLUDE_DIR
    NAMES adlmidi.h
    HINTS
    /usr/include
    /usr/local/include)

FIND_LIBRARY(AUDIO_ADLMIDI_LIBRARY
    NAMES ADLMIDI adlmidi
    HINTS
    /usr/lib
    /usr/local/lib
    /usr/lib/x86_64-linux-gnu
    ${LIB_3RD_FIND_HINT})

IF(AUDIO_ADLMIDI_INCLUDE_DIR AND AUDIO_ADLMIDI_LIBRARY)
    MESSAGE(STATUS "Found libADLMIDI: ${AUDIO_ADLMIDI_LIBRARY}")
    INCLUDE_DIRECTORIES(${AUDIO_ADLMIDI_INCLUDE_DIR})
    
    # Use the same plugin name as other MIDI plugins so only one is active
    add_cm_plugin(CMP.Audio.ADLMIDI "Plug-Ins//Audio" MidiRead.cpp ${AUDIO_ADLMIDI_LIBRARY})
ELSE()
    MESSAGE(STATUS "libADLMIDI not found. ADLMIDI plugin will not be built.")
    MESSAGE(STATUS "  To enable ADLMIDI (OPL3/AdLib chip emulation) support:")
    MESSAGE(STATUS "  Ubuntu/Debian: sudo apt-get install libadlmidi-dev")
    MESSAGE(STATUS "  Or build from: https://github.com/Wohlstand/libADLMIDI")
    MESSAGE(STATUS "  Note: This provides authentic DOS game/AdLib sound")
ENDIF()
