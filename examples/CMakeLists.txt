# AudioMixer Examples
cmake_minimum_required(VERSION 3.10)

# Include parent project paths
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Link to CMAudio library
link_directories(${CMAKE_BINARY_DIR}/src)

# Basic AudioMixer test
add_executable(mixer_basic_test
    mixer_basic_test.cpp
)
target_link_libraries(mixer_basic_test CMAudio)

# AudioMixerScene city test
add_executable(scene_city_test
    scene_city_test.cpp
)
target_link_libraries(scene_city_test CMAudio)

# AudioMixerScene swarm test
add_executable(scene_swarm_test
    scene_swarm_test.cpp
)
target_link_libraries(scene_swarm_test CMAudio)

# WAV resample tool
add_executable(wav_resample
    wav_resample.cpp
)
target_link_libraries(wav_resample CMAudio)

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
