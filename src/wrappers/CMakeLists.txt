# CMakeLists.txt for Maximilian C wrapper
cmake_minimum_required(VERSION 3.15)

# Create the wrapper library
add_library(maximilian_wrapper STATIC maxi_wrapper.cpp)

# Set C++ standard
set_target_properties(maximilian_wrapper PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# Link with Maximilian library
target_link_libraries(maximilian_wrapper PUBLIC maximilian)

# Include directories
target_include_directories(maximilian_wrapper PUBLIC
    ${CMAUDIO_ROOT_INCLUDE_PATH}
)

# Compile options
if(MSVC)
    target_compile_options(maximilian_wrapper PRIVATE /W4)
else()
    target_compile_options(maximilian_wrapper PRIVATE -Wall -Wextra)
endif()
