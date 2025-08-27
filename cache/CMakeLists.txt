cmake_minimum_required(VERSION 3.14)
project(StridesBenchmark LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific flags
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Check if strides_benchmark.cpp has content
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/strides_benchmark.cpp" STRIDES_BENCHMARK_CONTENT)
string(LENGTH "${STRIDES_BENCHMARK_CONTENT}" CONTENT_LENGTH)

if(CONTENT_LENGTH GREATER 0)
    # Build strides benchmark executable
    add_executable(strides_benchmark strides_benchmark.cpp)
    
    # Set specific compiler flags for this target
    target_compile_features(strides_benchmark PRIVATE cxx_std_20)
    
    # Optional: Add installation rule
    install(TARGETS strides_benchmark DESTINATION bin)
else()
    message(WARNING "strides_benchmark.cpp is empty. No executable will be built.")
endif()
