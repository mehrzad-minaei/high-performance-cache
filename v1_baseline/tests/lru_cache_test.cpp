#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>
#include "lru_cache.hpp"

using namespace cache::v1;

// --- Basic Functionality Tests ---

TEST(LRUCacheTest, BasicPutAndGet) {
    LRUCache<int, std::string> cache(2);

    cache.put(1, "one");
    cache.put(2, "two");

    EXPECT_EQ(cache.get(1), "one");
    EXPECT_EQ(cache.get(2), "two");
    EXPECT_EQ(cache.get(3), std::nullopt); // Non-existent key
}

TEST(LRUCacheTest, UpdateExistingKey) {
    LRUCache<int, std::string> cache(2);

    cache.put(1, "one");
    cache.put(1, "updated_one");

    EXPECT_EQ(cache.get(1), "updated_one");
    EXPECT_EQ(cache.size(), 1);
}

TEST(LRUCacheTest, EvictionPolicy) {
    LRUCache<int, std::string> cache(2);

    cache.put(1, "one");
    cache.put(2, "two");

    // Key 1 is accessed; Key 2 becomes the least recently used (LRU)
    EXPECT_EQ(cache.get(1), "one");

    // Insert Key 3 -> Key 2 should be evicted
    cache.put(3, "three");

    EXPECT_EQ(cache.get(1), "one");
    EXPECT_EQ(cache.get(2), std::nullopt); // Evicted
    EXPECT_EQ(cache.get(3), "three");
}

TEST(LRUCacheTest, RemoveKey) {
    LRUCache<int, std::string> cache(2);

    cache.put(1, "one");
    cache.put(2, "two");

    EXPECT_TRUE(cache.remove(1));
    EXPECT_FALSE(cache.remove(1)); // Already removed
    EXPECT_EQ(cache.get(1), std::nullopt);
    EXPECT_EQ(cache.size(), 1);
}

TEST(LRUCacheTest, ClearCache) {
    LRUCache<int, std::string> cache(3);

    cache.put(1, "one");
    cache.put(2, "two");
    cache.clear();

    EXPECT_EQ(cache.size(), 0);
    EXPECT_EQ(cache.get(1), std::nullopt);
    EXPECT_EQ(cache.get(2), std::nullopt);
}

TEST(LRUCacheTest, ZeroCapacity) {
    LRUCache<int, std::string> cache(0);

    cache.put(1, "one");
    EXPECT_EQ(cache.size(), 0);
    EXPECT_EQ(cache.get(1), std::nullopt);
}

// --- Move Semantics Tests ---

TEST(LRUCacheTest, MoveConstructor) {
    LRUCache<int, std::string> cache1(2);
    cache1.put(1, "one");
    cache1.put(2, "two");

    LRUCache<int, std::string> cache2(std::move(cache1));

    EXPECT_EQ(cache2.size(), 2);
    EXPECT_EQ(cache2.get(1), "one");
    EXPECT_EQ(cache2.get(2), "two");
    
    // Original cache should be empty
    EXPECT_EQ(cache1.size(), 0);
    EXPECT_EQ(cache1.get(1), std::nullopt);
}

TEST(LRUCacheTest, MoveAssignment) {
    LRUCache<int, std::string> cache1(2);
    cache1.put(1, "one");

    LRUCache<int, std::string> cache2(2);
    cache2.put(99, "ninety-nine");

    cache2 = std::move(cache1);

    EXPECT_EQ(cache2.size(), 1);
    EXPECT_EQ(cache2.get(1), "one");
    EXPECT_EQ(cache2.get(99), std::nullopt);
}

// --- Concurrency Test ---

TEST(LRUCacheTest, ConcurrentPutAndGet) {
    constexpr size_t capacity = 1000;
    constexpr int num_threads = 8;
    constexpr int ops_per_thread = 2000;

    LRUCache<int, int> cache(capacity);
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&cache, t, ops_per_thread]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                int key = (t * ops_per_thread + i) % static_cast<int>(capacity * 1.5);
                cache.put(key, key);
                auto val = cache.get(key);
                if (val.has_value()) {
                    EXPECT_EQ(val.value(), key);
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_LE(cache.size(), capacity);
}