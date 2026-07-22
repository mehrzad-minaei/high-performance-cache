# Header-only template library target
add_library(v1_cache INTERFACE)

target_include_directories(v1_cache INTERFACE include)

# Build tests if enabled at root
if(BUILD_TESTING)
    add_executable(v1_baseline_tests tests/lru_cache_test.cpp)
    target_link_libraries(v1_baseline_tests PRIVATE v1_cache GTest::gtest_main)

    # Register tests with CTest
    gtest_discover_tests(v1_baseline_tests)
endif()