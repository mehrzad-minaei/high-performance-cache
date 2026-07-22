cmake_minimum_required(VERSION 3.20)
project(HighPerformanceCache LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to toggle building unit tests globally
option(BUILD_TESTING "Build unit tests" ON)

if(BUILD_TESTING)
    enable_testing()

    # Centrally fetch and configure Google Test
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    # Ensure compatible runtime library settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # Enables gtest_discover_tests() across all subdirectories
    include(GoogleTest)
endif()

# Subdirectories
add_subdirectory(v1_baseline)
add_subdirectory(benchmarks)