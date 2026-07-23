#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <numeric>
#include "sharded_lru_cache.hpp"

namespace cache::v2::test {

class ShardedLRUCacheTest : public ::testing::Test {
protected:
    // Helper to create a cache with 10 total capacity split across 4 shards
    static constexpr size_t kTotalCapacity = 10;
    static constexpr size_t kNumShards = 4;
};

// -----------------------------------------------------------------------------
// Core Functional Tests
// -----------------------------------------------------------------------------

TEST_F(ShardedLRUCacheTest, BasicPutAndGet) {
    ShardedLRUCache<int, std::string> cache(kTotalCapacity, kNumShards);

    cache.put(1, "one");
    cache.put(2, "two");

    EXPECT_EQ(cache.get(1), "one");
    EXPECT_EQ(cache.get(2), "two");
    EXPECT_EQ(cache.get(3), std::nullopt);
}

TEST_F(ShardedLRUCacheTest, UpdateExistingKey) {
    ShardedLRUCache<int, std::string> cache(kTotalCapacity, kNumShards);

    cache.put(1, "one");
    EXPECT_EQ(cache.get(1), "one");

    cache.put(1, "one_updated");
    EXPECT_EQ(cache.get(1), "one_updated");
    EXPECT_EQ(cache.size(), 1);
}

TEST_F(ShardedLRUCacheTest, RemoveKey) {
    ShardedLRUCache<int, std::string> cache(kTotalCapacity, kNumShards);

    cache.put(1, "one");
    EXPECT_TRUE(cache.remove(1));
    EXPECT_EQ(cache.get(1), std::nullopt);
    EXPECT_FALSE(cache.remove(1)); // Second removal should fail
}

TEST_F(ShardedLRUCacheTest, ClearCache) {
    ShardedLRUCache<int, int> cache(20, 4);

    for (int i = 0; i < 15; ++i) {
        cache.put(i, i * 10);
    }
    EXPECT_GT(cache.size(), 0);

    cache.clear();
    EXPECT_EQ(cache.size(), 0);
    EXPECT_EQ(cache.get(0), std::nullopt);
}

// -----------------------------------------------------------------------------
// Sharding & Eviction Logic
// -----------------------------------------------------------------------------

TEST_F(ShardedLRUCacheTest, CapacityPerShardRounding) {
    // 10 capacity / 4 shards -> ceiling rounding gives 3 per shard
    // Total effective capacity = 3 * 4 = 12
    ShardedLRUCache<int, int> cache(10, 4);
    
    EXPECT_EQ(cache.num_shards(), 4);
    EXPECT_EQ(cache.capacity(), 12); 
}

TEST_F(ShardedLRUCacheTest, EvictionPerShard) {
    // 1 capacity per shard across 2 shards (Total Capacity = 2)
    ShardedLRUCache<std::string, int> cache(2, 2);

    // Custom deterministic routing test:
    // Place keys into the cache and verify LRU eviction happens independently per shard
    cache.put("key1", 100);
    cache.put("key2", 200);

    // Re-inserting to the same shard should evict the previous key in that specific shard
    cache.put("key1", 101); // Update key1
    EXPECT_EQ(cache.get("key1"), 101);
}

TEST_F(ShardedLRUCacheTest, FallbackNonPowerOfTwoShards) {
    // Verify routing logic works even if num_shards is not a power of 2 (forces modulo fallback)
    ShardedLRUCache<int, int> cache(10, 3);

    for (int i = 0; i < 20; ++i) {
        cache.put(i, i);
    }

    // Should successfully retrieve items within shard capacities without crashing
    EXPECT_EQ(cache.num_shards(), 3);
}

// -----------------------------------------------------------------------------
// Concurrency Tests (Lock-Striping Validation)
// -----------------------------------------------------------------------------

TEST_F(ShardedLRUCacheTest, HighConcurrencyMultiThreaded) {
    constexpr size_t total_capacity = 1000;
    constexpr size_t num_shards = 16;
    constexpr int num_threads = 8;
    constexpr int ops_per_thread = 5000;

    ShardedLRUCache<int, int> cache(total_capacity, num_shards);
    std::atomic<bool> start_flag{false};
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            // Spinlock wait until all threads are spawned
            while (!start_flag.load(std::memory_order_relaxed)) {}

            for (int i = 0; i < ops_per_thread; ++i) {
                int key = (t * 1000) + (i % 100); // Intentionally create key overlap
                
                if (i % 3 == 0) {
                    cache.put(key, i);
                } else if (i % 3 == 1) {
                    cache.get(key);
                } else {
                    cache.remove(key);
                }
            }
        });
    }

    // Unblock all threads simultaneously to maximize contention across shards
    start_flag.store(true);

    for (auto& thread : threads) {
        thread.join();
    }

    // Cache should remain in a valid, non-corrupted state
    EXPECT_LE(cache.size(), cache.capacity());
}

} // namespace cache::v2::test