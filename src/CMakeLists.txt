set(CM_OPENAL_HEADER    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/al/al.h
                        ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/al/alc.h
                        ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/al/efx.h
                        ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/al/efx-presets.h
                        ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/al/efx-creative.h
                        ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/al/xram.h)

set(CM_AUDIO_HEADER ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioBuffer.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioAnalysis.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioBank.h
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
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioMixerScene.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioClient.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioFilter.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioMemoryPool.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Gain.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/InterpolationType.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/SplitComplex.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioFFT.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Convolver.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/AmbisonicTypes.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/AmbisonicComponent.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/BFormat.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/AmbisonicEntity.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/AmbisonicSource.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/AmbisonicSpeaker.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/AmbisonicDecoder.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/AmbisonicOrientationProcessor.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/AmbisonicShelfFilter.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/AmbisonicBinauralizer.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/LinkwitzRileyFilter.h
                    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/HRTF/HRIRSphere.h)

set(CM_OPENAL_SOURCE    al.cpp
                        alc.cpp
                        EFX.cpp
                        XRAM.cpp)

set(CM_AUDIO_SOURCE
    OpenAL.cpp
    AudioFileType.cpp
    AudioBank.cpp
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
    AudioMixerScene.cpp
    AudioFFT.cpp
    Convolver.cpp
    pffft/pffft.cpp
    pffft/pffft_common.cpp
    Ambisonics/AmbisonicDecoder.cpp
    HRTF/HRIRSphere.cpp)

# ===== source_group 按功能域分组（VS 解决方案内组织）=====

# 1. OpenAL：底层绑定与驱动
source_group("OpenAL" FILES ${CM_OPENAL_HEADER}
                          ${CM_OPENAL_SOURCE}
                          ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/OpenAL.h
                          OpenAL.cpp)

# 2. Core：引擎核心 / 播放 / 资源 / 会话
source_group("Core" FILES
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioEngine.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioManager.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioPlayer.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioSource.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioBus.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioBuffer.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioFileType.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioAssetManager.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioMemoryPool.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Gain.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/GainEnvelope.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/DynamicMusic.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioSessionPolicy.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioDecode.h
    AudioEngine.cpp
    AudioManager.cpp
    AudioPlayer.cpp
    AudioSource.cpp
    AudioBus.cpp
    AudioBuffer.cpp
    AudioFileType.cpp
    AudioAssetManager.cpp
    DynamicMusic.cpp
    AudioSessionPolicy.cpp
    AudioDecode.cpp)

# 3. EventCue：事件指令协议 / 传输层 / 引擎线程 / Cue 管理 / C API
source_group("EventCue" FILES
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioEvent.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/EventTransport.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/IPCTransport.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioEngineThread.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/SoundEvent.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/SoundEventManager.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioClient.h
    AudioEvent.cpp
    IPCTransport.cpp
    AudioEngineThread.cpp
    SoundEvent.cpp
    SoundEventManager.cpp)

# 4. Capture：录音采集
source_group("Capture" FILES
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioCapture.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/CaptureSource.h
    AudioCapture.cpp
    CaptureSource.cpp)

# 5. VoiceFX：变声器（实时 + 离线）
source_group("VoiceFX" FILES
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/WSOLAShifter.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/LinearResampler.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/PitchShifter.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/VoiceChain.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/OfflinePitchShift.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/FormantCorrector.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/OfflineVoiceFX.h
    WSOLAShifter.cpp
    LinearResampler.cpp
    PitchShifter.cpp
    VoiceChain.cpp
    OfflinePitchShift.cpp
    FormantCorrector.cpp
    OfflineVoiceFX.cpp)

# 6. VoiceCall：语音通话链（编解码 / 预处理 / 抖动缓冲）
source_group("VoiceCall" FILES
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioCodec.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/VoicePreprocess.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/JitterBuffer.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/VoiceCall.h
    AudioCodec.cpp
    VoicePreprocess.cpp
    JitterBuffer.cpp
    VoiceCall.cpp)

# 7. DSP：滤波 / EQ / 动态 / 响度 / 分析
source_group("DSP" FILES
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/BiquadFilter.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Compressor.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioEQ.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/ParametricEQ.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/TimeEffects.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/LoudnessNormalizer.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioAnalysis.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioFilterPreset.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioFilter.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/ReverbPreset.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/InterpolationType.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/SplitComplex.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioFFT.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Convolver.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/AmbisonicTypes.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/AmbisonicComponent.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/BFormat.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/AmbisonicEntity.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/AmbisonicSource.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/AmbisonicSpeaker.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/AmbisonicDecoder.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/AmbisonicOrientationProcessor.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/AmbisonicShelfFilter.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/AmbisonicBinauralizer.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/Ambisonics/LinkwitzRileyFilter.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/HRTF/HRIRSphere.h
    BiquadFilter.cpp
    Compressor.cpp
    AudioEQ.cpp
    ParametricEQ.cpp
    LoudnessNormalizer.cpp
    AudioAnalysis.cpp
    AudioFilterPreset.cpp
    ReverbPreset.cpp
    AudioFFT.cpp
    Convolver.cpp
    Ambisonics/AmbisonicDecoder.cpp)

# 8. Mixer：混音 / 重采样 / 场景
source_group("Mixer" FILES
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioMixer.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioResampler.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioMixerScene.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioMixerTypes.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioMixerSourceConfig.h
    AudioMixer.cpp
    AudioResampler.cpp
    AudioMixerScene.cpp)

# 9. MIDI：乐器 / 播放器 / 管弦乐
source_group("MIDI" FILES
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/MIDIInstrument.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/MIDIPlayer.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/MIDIOrchestraPlayer.h
    MIDIInstrument.cpp
    MIDIPlayer.cpp
    MIDIOrchestraPlayer.cpp)

# 10. Spatial：3D 空间音频
source_group("Spatial" FILES
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/SpatialAudioWorld.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/DirectionalGainPattern.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/ConeAngle.h
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioListener.h
    SpatialAudioWorld.cpp
    DirectionalGainPattern.cpp
    Listener.cpp)

# 11. Bank：音频银行打包
source_group("Bank" FILES
    ${CMAUDIO_ROOT_INCLUDE_PATH}/hgl/audio/AudioBank.h
    AudioBank.cpp)

# ---- AudioClient DLL（T6：C API 客户端 SDK）----
add_cm_plugin(CMP.AudioClient "Plug-Ins//Audio" AudioClient.cpp)
target_link_libraries(CMP.AudioClient PRIVATE CMAudio CMCore)

add_cm_library(CMAudio "CM" ${CMAUDIO_ROOT_INCLUDE_PATH} ${CM_OPENAL_HEADER}
                            ${CM_OPENAL_SOURCE}
                            ${CM_AUDIO_HEADER}
                            ${CM_AUDIO_SOURCE})

target_include_directories(CMAudio PRIVATE ${CMAUDIO_ROOT_SOURCE_PATH}
                                            ${CMAUDIO_ROOT_SOURCE_PATH}/Ambisonics)

target_link_libraries(CMAudio PRIVATE samplerate)