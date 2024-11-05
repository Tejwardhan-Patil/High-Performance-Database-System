#include <iostream>
#include <unordered_map>
#include <list>
#include <cassert>

// Constants
constexpr size_t CACHE_CAPACITY = 100; 

// Data block structure
struct DataBlock {
    int key;
    int value;
    bool dirty; // Indicates if the block has been modified (write-back mechanism)
    DataBlock(int k, int v) : key(k), value(v), dirty(false) {}
};

// Write-back Cache class
class WriteBackCache {
private:
    // Doubly linked list to maintain LRU order
    std::list<DataBlock> cache_list;
    // Hash map to store references of keys to the list nodes
    std::unordered_map<int, std::list<DataBlock>::iterator> cache_map;
    // The size of the cache
    size_t capacity;
    // Simulating write to the underlying storage
    void write_to_storage(int key, int value) {
        std::cout << "Writing data block (key=" << key << ", value=" << value << ") to storage.\n";
    }
    
    // Evicts the least recently used data block from the cache
    void evict() {
        auto evict_block = cache_list.back();
        if (evict_block.dirty) {
            // If the block is dirty, write it back to the storage
            write_to_storage(evict_block.key, evict_block.value);
        }
        cache_map.erase(evict_block.key);
        cache_list.pop_back();
    }

public:
    // Constructor to initialize the cache with a given capacity
    WriteBackCache(size_t capacity) : capacity(capacity) {}

    // Read data block from the cache
    int read(int key) {
        if (cache_map.find(key) == cache_map.end()) {
            std::cout << "Data block (key=" << key << ") not found in cache. Fetching from storage.\n";
            return -1; // Simulating cache miss and fetching from storage
        }
        // Move the accessed block to the front (mark as most recently used)
        cache_list.splice(cache_list.begin(), cache_list, cache_map[key]);
        return cache_map[key]->value;
    }

    // Write data block to the cache
    void write(int key, int value) {
        if (cache_map.find(key) != cache_map.end()) {
            // If the block is already in cache, update it and mark it as dirty
            auto it = cache_map[key];
            it->value = value;
            it->dirty = true;
            // Move to the front (most recently used)
            cache_list.splice(cache_list.begin(), cache_list, it);
        } else {
            // If cache is full, evict the least recently used block
            if (cache_list.size() >= capacity) {
                evict();
            }
            // Insert the new block at the front of the list
            cache_list.emplace_front(key, value);
            cache_map[key] = cache_list.begin();
        }
    }

    // Flush all dirty blocks to storage
    void flush() {
        for (auto &block : cache_list) {
            if (block.dirty) {
                write_to_storage(block.key, block.value);
                block.dirty = false;
            }
        }
    }

    // Display cache status for debugging
    void display_cache() {
        std::cout << "Cache Status [Key:Value:Dirty] -> ";
        for (const auto &block : cache_list) {
            std::cout << "[" << block.key << ":" << block.value << ":" << (block.dirty ? "D" : "C") << "] ";
        }
        std::cout << std::endl;
    }
};

int main() {
    // Initialize write-back cache with given capacity
    WriteBackCache cache(CACHE_CAPACITY);

    // Simulating write operations to the cache
    cache.write(1, 100);
    cache.write(2, 200);
    cache.write(3, 300);
    cache.display_cache();

    // Read from cache
    std::cout << "Read key 2: " << cache.read(2) << std::endl;
    cache.display_cache();

    // Write to an existing block (update and mark as dirty)
    cache.write(2, 250);
    cache.display_cache();

    // Simulating cache overflow
    for (int i = 4; i <= 105; ++i) {
        cache.write(i, i * 100);
    }
    cache.display_cache();

    // Flushing all dirty blocks to storage
    std::cout << "Flushing dirty blocks...\n";
    cache.flush();
    cache.display_cache();

    return 0;
}