# Examples CMakeLists.txt
cmake_minimum_required(VERSION 3.15)

# Only build examples if WITH_MAXIMILIAN is ON
if(WITH_MAXIMILIAN)
    add_subdirectory(maximilian_demo)
endif()
