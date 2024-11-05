#include <iostream>
#include <unordered_map>
#include <list>

class LRUCache {
private:
    // Define the maximum capacity of the cache
    int capacity;
    
    // List to store the keys in order of usage (most recent at the front)
    std::list<int> lruList;
    
    // Hash map to store key-value pairs and their positions in the list
    std::unordered_map<int, std::pair<int, std::list<int>::iterator>> cache;

    // Move a key to the front of the LRU list (most recently used)
    void moveToFront(int key) {
        lruList.erase(cache[key].second);
        lruList.push_front(key); 
        cache[key].second = lruList.begin(); 
    }

public:
    // Constructor to initialize the LRU cache with a specific capacity
    LRUCache(int capacity) {
        this->capacity = capacity;
    }

    // Function to get the value of a key from the cache
    int get(int key) {
        if (cache.find(key) == cache.end()) {
            return -1; 
        } else {
            moveToFront(key);
            return cache[key].first; 
        }
    }

    // Function to insert a key-value pair into the cache
    void put(int key, int value) {
        if (cache.find(key) != cache.end()) {
            // If the key already exists, update its value and move it to the front
            cache[key].first = value;
            moveToFront(key);
        } else {
            // If the key is new, check if the cache is at capacity
            if (cache.size() == capacity) {
                int evictKey = lruList.back();
                lruList.pop_back(); 
                cache.erase(evictKey); 
            }
            // Insert the new key-value pair
            lruList.push_front(key);
            cache[key] = {value, lruList.begin()};
        }
    }

    // Function to display the current state of the cache
    void displayCache() {
        std::cout << "Cache contents (most recent to least recent): ";
        for (auto it = lruList.begin(); it != lruList.end(); ++it) {
            std::cout << *it << " ";
        }
        std::cout << std::endl;
    }
};

int main() {
    // Initialize an LRUCache object with a capacity of 3
    LRUCache cache(3);

    // Perform cache operations
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    std::cout << "Initial cache state: ";
    cache.displayCache();

    std::cout << "Get key 1: " << cache.get(1) << std::endl; 
    cache.displayCache(); 

    cache.put(4, 40); 
    std::cout << "After inserting key 4: ";
    cache.displayCache();

    std::cout << "Get key 2: " << cache.get(2) << std::endl; 
    cache.put(5, 50); 
    std::cout << "After inserting key 5: ";
    cache.displayCache();

    return 0;
}