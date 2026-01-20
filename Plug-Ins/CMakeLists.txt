INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/libogg/include)

SET(OGG_SOURCE	${CMAKE_CURRENT_SOURCE_DIR}/libogg/src/bitwise.c
				${CMAKE_CURRENT_SOURCE_DIR}/libogg/src/framing.c)

add_subdirectory(Audio.Wav)
add_subdirectory(Audio.Opus)
add_subdirectory(Audio.WildMidi)

IF(AUDIO_VORBIS_DECODE_LIBRARY STREQUAL TREMOLO)
	add_subdirectory(Audio.Tremolo)
ELSEIF(AUDIO_VORBIS_DECODE_LIBRARY STREQUAL TREMOR)
	add_subdirectory(Audio.Tremor)
ELSE()
	add_subdirectory(Audio.Vorbis)
ENDIF()
