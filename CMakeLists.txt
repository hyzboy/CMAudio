﻿cmake_minimum_required(VERSION 3.0)

project(CMAudio)

include(path_config.cmake)
CMAudioSetup(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(${CMAUDIO_ROOT_SOURCE_PATH})
