cmake_minimum_required(VERSION 3.10)
project(Maximilian)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create the library
add_library(maximilian STATIC
    # Main source files
    Maximilian/src/maximilian.cpp

    # Library components
    Maximilian/src/libs/fft.cpp
    Maximilian/src/libs/maxiAtoms.cpp
    Maximilian/src/libs/maxiBark.cpp
    Maximilian/src/libs/maxiConvolve.cpp
    Maximilian/src/libs/maxiFFT.cpp
    Maximilian/src/libs/maxiGrains.cpp
    Maximilian/src/libs/maxiMFCC.cpp
    Maximilian/src/libs/maxiReverb.cpp
    Maximilian/src/libs/maxiSynths.cpp
    Maximilian/src/libs/stb_vorbis.c
)

# Include directories
target_include_directories(maximilian PUBLIC
    Maximilian/src
    Maximilian/src/libs
)

# Optional: Add compiler flags for optimization
if(MSVC)
    target_compile_options(maximilian PRIVATE /W4)
else()
    target_compile_options(maximilian PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Export for use by other projects
set(MAXIMILIAN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Maximilian/src" CACHE PATH "Maximilian include directory" FORCE)
set(MAXIMILIAN_LIBRARIES maximilian CACHE STRING "Maximilian library" FORCE)
