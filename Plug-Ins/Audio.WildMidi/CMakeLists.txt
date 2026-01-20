IF(WIN32)
    # For Windows, we would need to include WildMIDI source or library
    # This is a placeholder for future Windows build support
    MESSAGE(STATUS "WildMIDI plugin: Windows build not yet configured. Please provide WildMIDI library.")
    
    # When WildMIDI source or library is available for Windows, uncomment and configure:
    # INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/wildmidi/include)
    # SET(WILDMIDI_SOURCE ...)
    # add_cm_plugin(CMP.Audio.MIDI "Plug-Ins//Audio" MidiRead.cpp ${WILDMIDI_SOURCE})
ELSE()
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
    ENDIF()
ENDIF()
