#pragma once

#include <functional>
#include <mutex>
#include <unordered_map>
#include <optional>
#include <utility>
#include <cstddef>

namespace cache::v1 {

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class LRUCache {
private:
    // Base node to act as dummy head/tail sentinels without requiring Key/Value default construction
    struct NodeBase {
        NodeBase* prev{nullptr};
        NodeBase* next{nullptr};
    };

    // Concrete node containing actual data
    struct Node : public NodeBase {
        Key key;
        Value value;
        Node(Key k, Value v) : key(std::move(k)), value(std::move(v)) {}
    };

public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {
        head_.next = &tail_;
        tail_.prev = &head_;
    }

    ~LRUCache() {
        clear();
    }

    // Disable copy semantics to prevent accidental copying of the mutex and cache structure
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;
    
    // Enable move semantics
    LRUCache(LRUCache&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.mutex_);
        capacity_ = other.capacity_;
        move_list_and_map(std::move(other));
    }

    LRUCache& operator=(LRUCache&& other) noexcept {
        if (this != &other) {
            std::scoped_lock lock(mutex_, other.mutex_);    
            clear_unlocked();
            capacity_ = other.capacity_;
            move_list_and_map(std::move(other));
        }
        return *this;
    }

    [[nodiscard]] std::optional<Value> get(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = map_.find(key);
        if (it == map_.end()) {
            return std::nullopt;
        }

        Node* node = it->second;
        detach(node);
        push_front(node);
        return node->value;
    }

    void put(const Key& key, Value value) {
        if (capacity_ == 0) return;

        std::lock_guard<std::mutex> lock(mutex_);

        auto it = map_.find(key);
        if (it != map_.end()) {
            // Update existing key: update value, move to front
            Node* node = it->second;
            node->value = std::move(value);
            detach(node);
            push_front(node);
            return;
        }

        // Handle cache eviction if capacity is reached
        if (map_.size() >= capacity_) {
            evict_oldest();
        }

        // Insert new node
        auto* new_node = new Node(key, std::move(value));
        push_front(new_node);
        map_[key] = new_node;
    }

    bool remove(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = map_.find(key);
        if (it == map_.end()) {
            return false;
        }

        Node* node = it->second;
        detach(node);
        map_.erase(it);
        delete node;
        return true;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        clear_unlocked();
    }

    [[nodiscard]] size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.size();
    }

    [[nodiscard]] size_t capacity() const {
        return capacity_;
    }

private:
    // Helper: Clears nodes assuming mutex_ is ALREADY locked by the caller
    void clear_unlocked() {
        NodeBase* curr = head_.next;
        while (curr != &tail_) {
            NodeBase* next = curr->next;
            delete static_cast<Node*>(curr);
            curr = next;
        }

        head_.next = &tail_;
        tail_.prev = &head_;
        map_.clear();
    }

    // Helper: Remove node from its current position in the list
    void detach(NodeBase* node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    // Helper: Push node to the head (most recently used position)
    void push_front(NodeBase* node) {
        node->next = head_.next;
        node->prev = &head_;
        head_.next->prev = node;
        head_.next = node;
    }

    // Helper: Evict the least recently used element (tail pointer)
    void evict_oldest() {
        if (tail_.prev == &head_) return; // List is empty

        Node* oldest = static_cast<Node*>(tail_.prev);
        detach(oldest);
        map_.erase(oldest->key);
        delete oldest;
    }

    // Helper: Move list state from another instance
    void move_list_and_map(LRUCache&& other) {
        map_ = std::move(other.map_);
        
        if (other.head_.next == &other.tail_) {
            head_.next = &tail_;
            tail_.prev = &head_;
        } else {
            head_.next = other.head_.next;
            head_.next->prev = &head_;
            tail_.prev = other.tail_.prev;
            tail_.prev->next = &tail_;
        }

        // Reset the moved-from instance
        other.head_.next = &other.tail_;
        other.tail_.prev = &other.head_;
        other.map_.clear();
    }

    size_t capacity_;
    mutable std::mutex mutex_;
    
    // Sentinels representing list boundaries
    NodeBase head_;
    NodeBase tail_;
    
    std::unordered_map<Key, Node*, Hash> map_;
};

} // namespace cache::v1