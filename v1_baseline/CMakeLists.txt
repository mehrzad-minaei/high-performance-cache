add_library(v1_cache 
    src/lru_cache.cpp
)

target_include_directories(v1_cache PUBLIC include)