cmake_minimum_required(VERSION 3.14)
project(GraphAdjacencyList LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Main library (header-only)
add_library(graph_adjacency_list INTERFACE)
target_include_directories(graph_adjacency_list INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Unit tests
enable_testing()
add_executable(graph_typed_graphs_test test_graph_typed_graphs.cpp)
target_link_libraries(graph_typed_graphs_test PRIVATE graph_adjacency_list GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(graph_typed_graphs_test)

# Benchmark app
add_executable(benchmark_graph benchmark_graph.cpp)
target_link_libraries(benchmark_graph PRIVATE graph_adjacency_list) 