cmake_minimum_required(VERSION 3.15)
project(HighPerformanceCache LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(v1_baseline)
add_subdirectory(benchmarks)