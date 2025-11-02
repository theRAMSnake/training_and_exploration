cmake_minimum_required(VERSION 3.14)
project(LocklessDataStructures LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_BUILD_TYPE Debug)

# GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Main library (header-only)
add_library(lockless_ordered_list INTERFACE)
target_include_directories(lockless_ordered_list INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(lockless_bounded_queue INTERFACE)
target_include_directories(lockless_bounded_queue INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(lockless_unbounded_queue INTERFACE)
target_include_directories(lockless_unbounded_queue INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(lockless_elimination_stack INTERFACE)
target_include_directories(lockless_elimination_stack INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(lockless_exchanger INTERFACE)
target_include_directories(lockless_exchanger INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(lockless_combining_tree INTERFACE)
target_include_directories(lockless_combining_tree INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Unit tests
enable_testing()
add_executable(test_list ordered_list/test_list.cpp)
target_link_libraries(test_list PRIVATE lockless_ordered_list GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(test_list)

add_executable(test_bounded_queue queue/test_bounded_queue.cpp)
target_link_libraries(test_bounded_queue PRIVATE lockless_bounded_queue GTest::gtest_main)
gtest_discover_tests(test_bounded_queue)

add_executable(test_unbounded_queue queue/test_unbounded_queue.cpp)
target_link_libraries(test_unbounded_queue PRIVATE lockless_unbounded_queue GTest::gtest_main)
gtest_discover_tests(test_unbounded_queue)

add_executable(test_elimination_stack stack/test_elimination_stack.cpp)
target_link_libraries(test_elimination_stack PRIVATE lockless_elimination_stack GTest::gtest_main)
gtest_discover_tests(test_elimination_stack)

add_executable(test_lockfree_exchanger stack/test_lockfree_exchanger.cpp)
target_link_libraries(test_lockfree_exchanger PRIVATE lockless_exchanger GTest::gtest_main)
gtest_discover_tests(test_lockfree_exchanger)

add_executable(test_combining_tree combining_tree/test_combining_tree.cpp)
target_link_libraries(test_combining_tree PRIVATE lockless_combining_tree GTest::gtest_main)
gtest_discover_tests(test_combining_tree)
