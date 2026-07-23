#pragma once

#include <functional>
#include <vector>
#include <memory>
#include <cstddef>
#include <optional>

// Include the baseline LRU cache for shard composition
#include "lru_cache.hpp"

namespace cache::v2 {

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class ShardedLRUCache {
public:
    /**
     * @param total_capacity Total capacity split across all shards.
     * @param num_shards Number of independent cache shards (should be a power of two for optimal hash routing).
     */
    ShardedLRUCache(size_t total_capacity, size_t num_shards = 16)
        : num_shards_(num_shards == 0 ? 1 : num_shards),
          shard_mask_((num_shards_ & (num_shards_ - 1)) == 0 ? num_shards_ - 1 : 0) {
        
        // Calculate per-shard capacity (rounding up to avoid loss of capacity)
        size_t per_shard_capacity = (total_capacity + num_shards_ - 1) / num_shards_;

        shards_.reserve(num_shards_);
        for (size_t i = 0; i < num_shards_; ++i) {
            shards_.push_back(std::make_unique<v1::LRUCache<Key, Value, Hash>>(per_shard_capacity));
        }
    }

    ~ShardedLRUCache() = default;

    // Non-copyable due to internal mutexes in shards
    ShardedLRUCache(const ShardedLRUCache&) = delete;
    ShardedLRUCache& operator=(const ShardedLRUCache&) = delete;

    // Movable
    ShardedLRUCache(ShardedLRUCache&&) noexcept = default;
    ShardedLRUCache& operator=(ShardedLRUCache&&) noexcept = default;

    [[nodiscard]] std::optional<Value> get(const Key& key) {
        return get_shard(key).get(key);
    }

    void put(const Key& key, Value value) {
        get_shard(key).put(key, std::move(value));
    }

    bool remove(const Key& key) {
        return get_shard(key).remove(key);
    }

    void clear() {
        for (auto& shard : shards_) {
            shard->clear();
        }
    }

    [[nodiscard]] size_t size() const {
        size_t total_size = 0;
        for (const auto& shard : shards_) {
            total_size += shard->size();
        }
        return total_size;
    }

    [[nodiscard]] size_t capacity() const {
        size_t total_cap = 0;
        for (const auto& shard : shards_) {
            total_cap += shard->capacity();
        }
        return total_cap;
    }

    [[nodiscard]] size_t num_shards() const noexcept {
        return num_shards_;
    }

private:
    [[nodiscard]] size_t get_shard_index(const Key& key) const {
        size_t hash_val = hasher_(key);

        // Fast bitwise AND routing if num_shards is a power of 2, otherwise fallback to modulo
        if (shard_mask_ != 0) {
            return hash_val & shard_mask_;
        }
        return hash_val % num_shards_;
    }

    [[nodiscard]] v1::LRUCache<Key, Value, Hash>& get_shard(const Key& key) {
        return *shards_[get_shard_index(key)];
    }

    [[nodiscard]] const v1::LRUCache<Key, Value, Hash>& get_shard(const Key& key) const {
        return *shards_[get_shard_index(key)];
    }

    size_t num_shards_;
    size_t shard_mask_; // Used for fast modulo if num_shards is power-of-two
    Hash hasher_{};
    std::vector<std::unique_ptr<v1::LRUCache<Key, Value, Hash>>> shards_;
};

} // namespace cache::v2