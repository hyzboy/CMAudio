set(CM_OPENAL_HEADER    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/al/al.h
                        ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/al/alc.h
                        ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/al/efx.h
                        ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/al/efx-creative.h
                        ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/al/xram.h)

set(CM_AUDIO_HEADER ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioBuffer.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioAnalysis.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/BiquadFilter.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Compressor.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioEQ.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioCapture.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/CaptureSource.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioAssetManager.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioBus.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioEngine.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioFileType.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/DynamicMusic.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/GainEnvelope.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioFilterPreset.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioManager.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioPlayer.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/MIDIInstrument.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/MIDIPlayer.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/MIDIOrchestraPlayer.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/SpatialAudioWorld.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/SoundEvent.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/SoundEventManager.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioSource.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/ConeAngle.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/DirectionalGainPattern.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioListener.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioSessionPolicy.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/LoudnessNormalizer.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/ParametricEQ.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/TimeEffects.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/WSOLAShifter.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/LinearResampler.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/PitchShifter.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/VoiceChain.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/OfflinePitchShift.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/FormantCorrector.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/OfflineVoiceFX.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioCodec.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioEvent.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/EventTransport.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/IPCTransport.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioEngineThread.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/VoicePreprocess.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/JitterBuffer.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/VoiceCall.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/OpenAL.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/ReverbPreset.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioMixerTypes.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioMixer.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioResampler.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioMixerSourceConfig.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioMixerScene.h)

set(CM_OPENAL_SOURCE    al.cpp
                        alc.cpp
                        EFX.cpp
                        XRAM.cpp)

set(CM_AUDIO_SOURCE
    OpenAL.cpp
    AudioFileType.cpp
    AudioBuffer.cpp
    AudioAnalysis.cpp
    BiquadFilter.cpp
    Compressor.cpp
    WSOLAShifter.cpp
    LinearResampler.cpp
    PitchShifter.cpp
    VoiceChain.cpp
    OfflinePitchShift.cpp
    FormantCorrector.cpp
    OfflineVoiceFX.cpp
    AudioEvent.cpp
    AudioEngineThread.cpp
    IPCTransport.cpp
    AudioCodec.cpp
    VoicePreprocess.cpp
    JitterBuffer.cpp
    VoiceCall.cpp
    AudioEQ.cpp
    AudioCapture.cpp
    CaptureSource.cpp
    AudioAssetManager.cpp
    AudioBus.cpp
    AudioEngine.cpp
    DynamicMusic.cpp
    AudioFilterPreset.cpp
    AudioDecode.cpp
    AudioManager.cpp
    AudioSessionPolicy.cpp
    SpatialAudioWorld.cpp
    DirectionalGainPattern.cpp
    LoudnessNormalizer.cpp
    ParametricEQ.cpp
    SoundEvent.cpp
    SoundEventManager.cpp
    ReverbPreset.cpp
    AudioPlayer.cpp
    MIDIInstrument.cpp
    MIDIPlayer.cpp
    MIDIOrchestraPlayer.cpp
    AudioSource.cpp
    Listener.cpp
    AudioMixer.cpp
    AudioResampler.cpp
    AudioMixerScene.cpp)

source_group("OpenAL" FILES ${CM_OPENAL_HEADER}
                            ${CM_OPENAL_SOURCE})

source_group("Audio" FILES  ${CM_AUDIO_HEADER}
                            ${CM_AUDIO_SOURCE})

# ---- AudioClient DLL（T6：C API 客户端 SDK）----
add_cm_plugin(CMP.AudioClient "Plug-Ins//Audio" AudioClient.cpp)
target_link_libraries(CMP.AudioClient PRIVATE CMAudio CMCore)

add_cm_library(CMAudio "CM" ${CMAUDIO_ROOT_INCLUDE_PATH} ${CM_OPENAL_HEADER}
                            ${CM_OPENAL_SOURCE}
                            ${CM_AUDIO_HEADER}
                            ${CM_AUDIO_SOURCE})

target_link_libraries(CMAudio PRIVATE samplerate)
