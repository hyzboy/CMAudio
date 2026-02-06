set(LIBTIMIDITY_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/libtimidity/src)

if(EXISTS ${LIBTIMIDITY_SRC_DIR}/timidity.h)
    file(GLOB LIBTIMIDITY_SOURCES "${LIBTIMIDITY_SRC_DIR}/*.c")
    add_library(libtimidity STATIC ${LIBTIMIDITY_SOURCES})
    target_compile_definitions(libtimidity PRIVATE TIMIDITY_STATIC)
    target_include_directories(libtimidity PUBLIC ${LIBTIMIDITY_SRC_DIR})

    set(AUDIO_TIMIDITY_LIBRARY libtimidity)
    set(AUDIO_TIMIDITY_INCLUDE_DIR ${LIBTIMIDITY_SRC_DIR})
else()
    # Try to find system libtimidity library
    FIND_PATH(AUDIO_TIMIDITY_INCLUDE_DIR
        NAMES timidity.h
        HINTS
        /usr/include
        /usr/local/include
        /usr/include/libtimidity)

    FIND_LIBRARY(AUDIO_TIMIDITY_LIBRARY
        NAMES timidity
        HINTS
        /usr/lib
        /usr/local/lib
        /usr/lib/x86_64-linux-gnu
        ${LIB_3RD_FIND_HINT})
endif()

IF(AUDIO_TIMIDITY_INCLUDE_DIR AND AUDIO_TIMIDITY_LIBRARY)
    MESSAGE(STATUS "Found libtimidity: ${AUDIO_TIMIDITY_LIBRARY}")
    INCLUDE_DIRECTORIES(${AUDIO_TIMIDITY_INCLUDE_DIR})
    
    # Use the same plugin name as WildMIDI so only one MIDI plugin is active
    add_cm_plugin(CMP.Audio.Timidity "Plug-Ins//Audio" MidiRead.cpp)

    target_link_libraries(CMP.Audio.Timidity PRIVATE ${AUDIO_TIMIDITY_LIBRARY})
    if(TARGET libtimidity)
        target_compile_definitions(CMP.Audio.Timidity PRIVATE TIMIDITY_STATIC)
    endif()
ELSE()
    MESSAGE(STATUS "libtimidity not found. Timidity MIDI plugin will not be built.")
    MESSAGE(STATUS "  To enable Timidity MIDI support, install libtimidity:")
    MESSAGE(STATUS "  Ubuntu/Debian: sudo apt-get install libtimidity-dev")
    MESSAGE(STATUS "  Fedora/RHEL:   sudo dnf install libtimidity-devel")
    MESSAGE(STATUS "  Note: This is an alternative to the WildMIDI plugin")
ENDIF()
