cmake_minimum_required(VERSION 3.28)

project(CMAudio)

# Option to build with Maximilian support
option(WITH_MAXIMILIAN "Build with Maximilian audio synthesis library support" ON)

include(path_config.cmake)
CMAudioSetup(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(${CMAUDIO_ROOT_SOURCE_PATH})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Plug-Ins)

# Conditionally build Maximilian integration
if(WITH_MAXIMILIAN)
    message(STATUS "Building with Maximilian support")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/Maximilian)
    
    # Build Maximilian wrapper if source exists
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/wrappers/maxi_wrapper.cpp)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/wrappers)
    endif()
    
    # Build examples if directory exists
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)
    endif()
else()
    message(STATUS "Building without Maximilian support (use -DWITH_MAXIMILIAN=ON to enable)")
endif()

