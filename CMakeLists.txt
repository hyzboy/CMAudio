cmake_minimum_required(VERSION 3.28)

project(CMAudio)

include(path_config.cmake)
CMAudioSetup(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(${CMAUDIO_ROOT_SOURCE_PATH})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Plug-Ins)

# Add examples (optional, can be disabled)
option(BUILD_EXAMPLES "Build AudioMixer examples" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)
endif()

