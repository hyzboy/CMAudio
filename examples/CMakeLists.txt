# AudioMixer Examples
cmake_minimum_required(VERSION 3.10)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# ============================================================================
# 与 CMCore examples 保持一致的示例工程宏：
#   统一 VC 工程文件夹为 Examples/CMAudio/<sub_folder>
#   参考 CMCore/examples/CMakeLists.txt 的 cm_example_project 封装
# ============================================================================
macro(cm_audio_example sub_folder project_name)
    add_executable(${project_name} ${ARGN})
    target_link_libraries(${project_name} PRIVATE CMAudio CMCore)

    # 设置 IDE 工程文件夹（与 CMCore 的 Examples/CMCore/... 风格一致）
    set_property(TARGET ${project_name} PROPERTY FOLDER "Examples/CMAudio/${sub_folder}")

    if(WIN32 AND COMMAND set_debugger_directory)
        # 调试运行目录指向构建目录（configs / wav_samples 已复制到那里）
        set_debugger_directory(${project_name} ${CMAKE_CURRENT_BINARY_DIR})
    endif()
endmacro()

# ---- 基础混音 ----
cm_audio_example("AudioMixer" mixer_basic_test mixer_basic_test.cpp)
cm_audio_example("AudioMixer" pitch_shift_test pitch_shift_test.cpp)

# ---- 场景混音 ----
cm_audio_example("AudioMixerScene" scene_city_test  scene_city_test.cpp)
cm_audio_example("AudioMixerScene" scene_swarm_test scene_swarm_test.cpp)

# ---- 重采样工具 ----
cm_audio_example("Resample" wav_resample wav_resample.cpp)

# ---- 音频总线 ----
cm_audio_example("AudioBus" bus_tree_test bus_tree_test.cpp)
cm_audio_example("AudioBus" bus_ducking_test bus_ducking_test.cpp)
cm_audio_example("AudioBus" sidechain_duck_test sidechain_duck_test.cpp)

# ---- 音频资源管理 ----
cm_audio_example("AudioAsset" asset_manager_test asset_manager_test.cpp)
cm_audio_example("AudioAsset" async_load_test async_load_test.cpp)

# ---- 音频引擎统一驱动 ----
cm_audio_example("AudioEngine" engine_update_test engine_update_test.cpp)

# ---- 声音事件数据驱动 ----
cm_audio_example("SoundEvent" sound_event_test sound_event_test.cpp)

# ---- 动态音乐分层 ----
cm_audio_example("DynamicMusic" dynamic_music_test dynamic_music_test.cpp)

# ---- 录音 ----
cm_audio_example("AudioCapture" audio_capture_test audio_capture_test.cpp)
cm_audio_example("AudioCapture" capture_player_test capture_player_test.cpp)

# ---- 语音通话编解码（编码插件接口 ver=5）----
cm_audio_example("VoiceCall" audio_codec_test audio_codec_test.cpp)
cm_audio_example("VoiceCall" voice_preprocess_test voice_preprocess_test.cpp)
cm_audio_example("VoiceCall" voice_call_test voice_call_test.cpp)

# ---- 音频分析（FFT/RMS/频谱/onset）----
cm_audio_example("AudioAnalysis" audio_analysis_test audio_analysis_test.cpp)

# ---- 通用双二阶滤波器（EQ 地基）----
cm_audio_example("BiquadFilter" biquad_test biquad_test.cpp)

# ---- 参数化均衡器（PEQ）----
cm_audio_example("ParametricEQ" param_eq_test param_eq_test.cpp)
cm_audio_example("ParametricEQ" eq_mixer_test eq_mixer_test.cpp)
cm_audio_example("ParametricEQ" audio_eq_test audio_eq_test.cpp)

# ---- 响度归一化（LUFS）----
cm_audio_example("Loudness" loudness_test loudness_test.cpp)
cm_audio_example("Loudness" loudness_limiter_test loudness_limiter_test.cpp)

# ---- 动态范围压缩器（Compressor/Limiter）----
cm_audio_example("Compressor" compressor_test compressor_test.cpp)

# ---- 时域效果（Delay/Echo/Chorus/Flanger）----
cm_audio_example("TimeEffects" time_effect_test time_effect_test.cpp)

# ---- 实时变声（WSOLA 变速 / PitchShifter 变调 / VoiceChain 预设）----
cm_audio_example("VoiceFX" voice_fx_test voice_fx_test.cpp)

# ---- 离线高质量变声（相位声码器 + formant 保持）----
cm_audio_example("VoiceFX" offline_voice_fx_test offline_voice_fx_test.cpp)

# ---- 移动平台会话策略 ----
cm_audio_example("AudioSession" session_policy_test session_policy_test.cpp)

# Copy configuration files to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/configs
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Create wav_samples directory in build
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/wav_samples)

# Copy README to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/wav_samples/README.md
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/wav_samples)

message(STATUS "AudioMixer examples configured")
message(STATUS "  - mixer_basic_test: Basic multi-track mixing")
message(STATUS "  - scene_city_test: City environment with TOML config")
message(STATUS "  - scene_swarm_test: Bee swarm with TOML config")
message(STATUS "")
message(STATUS "IMPORTANT: Place WAV samples in examples/wav_samples/")
message(STATUS "See examples/wav_samples/README.md for details")
