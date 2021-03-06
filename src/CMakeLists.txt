﻿file(GLOB CM_OPENAL_HEADER ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/al/*.*)
file(GLOB CM_AUDIO_HEADER ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/*.*)

set(CM_OPENAL_SOURCE    al.cpp
                        alc.cpp
                        EFX.cpp
                        XRAM.cpp)

set(CM_AUDIO_SOURCE
    OpenAL.cpp
    AudioFileType.cpp
    AudioBuffer.cpp
    AudioDecode.cpp
    AudioManage.cpp
    AudioScene.cpp
    AudioPlayer.cpp
    AudioSource.cpp
    Listener.cpp)

source_group("OpenAL" FILES ${CM_OPENAL_HEADER}
                            ${CM_OPENAL_SOURCE})

source_group("Audio" FILES  ${CM_AUDIO_HEADER}
                            ${CM_AUDIO_SOURCE})

add_cm_library(CMAudio "CM" ${CM_OPENAL_HEADER}
                            ${CM_OPENAL_SOURCE}
                            ${CM_AUDIO_HEADER}
                            ${CM_AUDIO_SOURCE})

#ADD_LIBRARY(CM.OpenALEE.StandAlone SHARED ${OpenALEESource} OpenALEECPort.cpp OpenALEEStandAlone.cpp ${CM_BASE_ALL_SOURCE})

#IF(WIN32)
#    target_compile_options(CM.OpenALEE.StandAlone PRIVATE "/MT")
#    target_link_libraries(CM.OpenALEE.StandAlone CM.Base.Static CM.Platform.Static)
#ENDIF(WIN32)
