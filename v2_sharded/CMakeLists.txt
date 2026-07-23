# Header-only template library target for Sharded LRU Cache
add_library(v2_sharded INTERFACE)

# Include directories for v2 headers
target_include_directories(v2_sharded INTERFACE include)

# v2 depends on v1's LRUCache interface target
target_link_libraries(v2_sharded INTERFACE v1_cache)

# Build tests if enabled at root
if(BUILD_TESTING)
    add_executable(v2_sharded_tests tests/sharded_lru_cache_test.cpp)
    target_link_libraries(v2_sharded_tests PRIVATE v2_sharded GTest::gtest_main)

    # Register tests with CTest
    gtest_discover_tests(v2_sharded_tests)
endif()