set(CM_OPENAL_HEADER    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/al/al.h
                        ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/al/alc.h
                        ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/al/efx.h
                        ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/al/efx-creative.h
                        ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/al/xram.h)

set(CM_AUDIO_HEADER ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioBuffer.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioFileType.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioManager.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioPlayer.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/SpatialAudioWorld.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioSource.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/ConeAngle.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/DirectionalGainPattern.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Listener.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/OpenAL.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/ReverbPreset.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioMixerTypes.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioMixer.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioScene.h)

set(CM_OPENAL_SOURCE    al.cpp
                        alc.cpp
                        EFX.cpp
                        XRAM.cpp)

set(CM_AUDIO_SOURCE
    OpenAL.cpp
    AudioFileType.cpp
    AudioBuffer.cpp
    AudioDecode.cpp
    AudioManager.cpp
    SpatialAudioWorld.cpp
    DirectionalGainPattern.cpp
    ReverbPreset.cpp
    AudioPlayer.cpp
    AudioSource.cpp
    Listener.cpp
    AudioMixer.cpp
    AudioScene.cpp)

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
