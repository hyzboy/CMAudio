# For Linux/Unix, try to find system WildMIDI library
FIND_PATH(AUDIO_WILDMIDI_INCLUDE_DIR
    NAMES wildmidi_lib.h
    HINTS
    /usr/include
    /usr/local/include)

FIND_LIBRARY(AUDIO_WILDMIDI_LIBRARY
    NAMES WildMidi wildmidi
    HINTS
    /usr/lib
    /usr/local/lib
    /usr/lib/x86_64-linux-gnu
    ${LIB_3RD_FIND_HINT})

IF(AUDIO_WILDMIDI_INCLUDE_DIR AND AUDIO_WILDMIDI_LIBRARY)
    MESSAGE(STATUS "Found WildMIDI: ${AUDIO_WILDMIDI_LIBRARY}")
    INCLUDE_DIRECTORIES(${AUDIO_WILDMIDI_INCLUDE_DIR})
    
    add_cm_plugin(CMP.Audio.MIDI "Plug-Ins//Audio" MidiRead.cpp ${AUDIO_WILDMIDI_LIBRARY})
ELSE()
    MESSAGE(STATUS "WildMIDI not found. MIDI plugin will not be built.")
    MESSAGE(STATUS "  To enable MIDI support, install WildMIDI:")
    MESSAGE(STATUS "  Ubuntu/Debian: sudo apt-get install libwildmidi-dev")
    MESSAGE(STATUS "  Fedora/RHEL:   sudo dnf install wildmidi-devel")
    MESSAGE(STATUS "  Windows:       Build or install WildMIDI library manually")
ENDIF()
