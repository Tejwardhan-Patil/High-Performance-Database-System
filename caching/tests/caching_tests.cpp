#include <iostream>
#include <unordered_map>
#include <list>
#include <cassert>

// LRU Cache class
class LRUCache {
public:
    LRUCache(int capacity) : capacity_(capacity) {}

    int get(int key) {
        if (cache_map_.find(key) == cache_map_.end()) {
            return -1; // Key not found
        }
        // Move accessed item to front (most recently used)
        cache_items_list_.splice(cache_items_list_.begin(), cache_items_list_, cache_map_[key]);
        return cache_map_[key]->second;
    }

    void put(int key, int value) {
        if (cache_map_.find(key) != cache_map_.end()) {
            // Update existing item and move to front
            cache_items_list_.splice(cache_items_list_.begin(), cache_items_list_, cache_map_[key]);
            cache_map_[key]->second = value;
            return;
        }

        if (cache_items_list_.size() >= capacity_) {
            // Evict least recently used item
            auto last = cache_items_list_.end();
            last--;
            cache_map_.erase(last->first);
            cache_items_list_.pop_back();
        }

        // Insert new item at the front
        cache_items_list_.emplace_front(key, value);
        cache_map_[key] = cache_items_list_.begin();
    }

private:
    int capacity_;
    std::list<std::pair<int, int>> cache_items_list_;
    std::unordered_map<int, std::list<std::pair<int, int>>::iterator> cache_map_;
};

// Test framework
void run_tests();

// Test functions
void test_lru_get();
void test_lru_put();
void test_lru_capacity();
void test_lru_eviction();
void test_lru_update();

// Main
int main() {
    run_tests();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}

// Test framework implementation
void run_tests() {
    test_lru_get();
    test_lru_put();
    test_lru_capacity();
    test_lru_eviction();
    test_lru_update();
}

// Test: Get operation on empty cache
void test_lru_get() {
    LRUCache cache(2);
    assert(cache.get(1) == -1); // Cache miss
    std::cout << "test_lru_get passed!" << std::endl;
}

// Test: Put operation and get the value back
void test_lru_put() {
    LRUCache cache(2);
    cache.put(1, 10);
    assert(cache.get(1) == 10); // Cache hit
    std::cout << "test_lru_put passed!" << std::endl;
}

// Test: Cache respects capacity limit
void test_lru_capacity() {
    LRUCache cache(2);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30); 
    assert(cache.get(1) == -1); 
    assert(cache.get(2) == 20); 
    assert(cache.get(3) == 30); 
    std::cout << "test_lru_capacity passed!" << std::endl;
}

// Test: Least Recently Used item is evicted
void test_lru_eviction() {
    LRUCache cache(2);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.get(1);    
    cache.put(3, 30); 
    assert(cache.get(1) == 10); 
    assert(cache.get(2) == -1); 
    assert(cache.get(3) == 30); 
    std::cout << "test_lru_eviction passed!" << std::endl;
}

// Test: Update existing cache item
void test_lru_update() {
    LRUCache cache(2);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(1, 15); 
    assert(cache.get(1) == 15); 
    assert(cache.get(2) == 20); 
    std::cout << "test_lru_update passed!" << std::endl;
}

// Additional test: Get operation moves item to most recent
void test_lru_recent_usage() {
    LRUCache cache(2);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.get(1);    
    cache.put(3, 30); 
    assert(cache.get(1) == 10); 
    assert(cache.get(2) == -1); 
    assert(cache.get(3) == 30); 
    std::cout << "test_lru_recent_usage passed!" << std::endl;
}

// Test: Cache eviction order correctness
void test_lru_eviction_order() {
    LRUCache cache(3);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    cache.get(1);   
    cache.get(2);   
    cache.put(4, 40); 
    assert(cache.get(1) == 10); 
    assert(cache.get(2) == 20); 
    assert(cache.get(3) == -1); 
    assert(cache.get(4) == 40); 
    std::cout << "test_lru_eviction_order passed!" << std::endl;
}