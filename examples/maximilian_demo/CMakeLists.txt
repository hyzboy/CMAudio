# CMakeLists.txt for Maximilian demo example
cmake_minimum_required(VERSION 3.15)

project(maximilian_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create the example executable
add_executable(maximilian_demo main_maxi.cpp)

# Link with wrapper library
target_link_libraries(maximilian_demo PRIVATE maximilian_wrapper)

# Include directories
target_include_directories(maximilian_demo PRIVATE
    ${CMAUDIO_ROOT_INCLUDE_PATH}
)

# Set output directory
set_target_properties(maximilian_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Platform-specific settings
if(UNIX AND NOT APPLE)
    # Linux - link pthread
    target_link_libraries(maximilian_demo PRIVATE pthread)
elseif(APPLE)
    # macOS - might need additional frameworks in future
    # target_link_libraries(maximilian_demo PRIVATE "-framework CoreAudio")
endif()

# Installation (optional)
install(TARGETS maximilian_demo
    RUNTIME DESTINATION bin
    COMPONENT examples
)
