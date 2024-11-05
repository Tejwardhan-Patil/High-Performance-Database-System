#include <iostream>
#include <unordered_map>
#include <mutex>
#include <fstream>

class Storage {
public:
    void write(int key, int value) {
        std::ofstream file("storage.txt", std::ios::app);
        if (file.is_open()) {
            file << key << ":" << value << std::endl;
            file.close();
        } else {
            throw std::runtime_error("Unable to open file for writing");
        }
    }

    int read(int key) {
        std::ifstream file("storage.txt");
        std::string line;
        if (file.is_open()) {
            while (std::getline(file, line)) {
                size_t pos = line.find(":");
                if (pos != std::string::npos) {
                    int file_key = std::stoi(line.substr(0, pos));
                    int file_value = std::stoi(line.substr(pos + 1));
                    if (file_key == key) {
                        return file_value;
                    }
                }
            }
            file.close();
        }
        return -1; 
    }
};

class WriteThroughCache {
public:
    WriteThroughCache(size_t capacity, Storage& storage) : capacity_(capacity), storage_(storage) {}

    int get(int key) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        if (cache_map_.find(key) != cache_map_.end()) {
            // Move the key to the front of the cache (for LRU)
            cache_list_.splice(cache_list_.begin(), cache_list_, cache_map_[key]);
            return cache_map_[key]->second;
        }

        // If not in cache, check the storage
        int value = storage_.read(key);
        if (value != -1) {
            put(key, value); 
        }
        return value;
    }

    void put(int key, int value) {
        std::lock_guard<std::mutex> lock(cache_mutex_);

        // Write through to storage first
        storage_.write(key, value);

        // If the key is already in the cache, update it
        if (cache_map_.find(key) != cache_map_.end()) {
            cache_map_[key]->second = value;
            cache_list_.splice(cache_list_.begin(), cache_list_, cache_map_[key]);
            return;
        }

        // Insert the new key-value pair into the cache
        cache_list_.emplace_front(key, value);
        cache_map_[key] = cache_list_.begin();

        // If exceed capacity, remove the least recently used item
        if (cache_list_.size() > capacity_) {
            auto last = cache_list_.end();
            --last;
            cache_map_.erase(last->first);
            cache_list_.pop_back();
        }
    }

private:
    size_t capacity_;
    std::list<std::pair<int, int>> cache_list_; // store cache items
    std::unordered_map<int, std::list<std::pair<int, int>>::iterator> cache_map_;
    Storage& storage_;
    std::mutex cache_mutex_;
};

// Testing the WriteThroughCache
int main() {
    Storage storage;
    WriteThroughCache cache(3, storage);

    // Test cache write-through mechanism
    cache.put(1, 100);
    cache.put(2, 200);
    cache.put(3, 300);

    std::cout << "Cache Get 1: " << cache.get(1) << std::endl;
    std::cout << "Cache Get 2: " << cache.get(2) << std::endl;

    cache.put(4, 400); 

    std::cout << "Cache Get 3 (should miss and read from storage): " << cache.get(3) << std::endl;
    std::cout << "Cache Get 4: " << cache.get(4) << std::endl;

    return 0;
}