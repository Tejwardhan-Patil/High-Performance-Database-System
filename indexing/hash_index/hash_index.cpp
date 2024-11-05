#include <iostream>
#include <vector>
#include <list>
#include <functional>
#include <stdexcept>
#include <memory>
#include <mutex>

// Bucket structure for each hash index bucket
template <typename Key, typename Value>
class HashBucket {
public:
    HashBucket() = default;
    ~HashBucket() = default;

    void insert(const Key& key, const Value& value) {
        for (auto& entry : bucket) {
            if (entry.first == key) {
                entry.second = value;  // Update if key exists
                return;
            }
        }
        bucket.emplace_back(key, value);  // Insert new key-value pair
    }

    bool find(const Key& key, Value& value) const {
        for (const auto& entry : bucket) {
            if (entry.first == key) {
                value = entry.second;
                return true;
            }
        }
        return false;
    }

    bool remove(const Key& key) {
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (it->first == key) {
                bucket.erase(it);
                return true;
            }
        }
        return false;
    }

private:
    std::list<std::pair<Key, Value>> bucket;  // Each bucket stores key-value pairs
};

// Thread-safe hash index structure
template <typename Key, typename Value>
class HashIndex {
public:
    HashIndex(size_t numBuckets, std::function<size_t(const Key&)> hashFunc)
        : numBuckets(numBuckets), hashFunc(hashFunc), buckets(numBuckets) {}

    // Insert key-value pair
    void insert(const Key& key, const Value& value) {
        size_t bucketIndex = hashFunc(key) % numBuckets;
        std::lock_guard<std::mutex> lock(mutexes[bucketIndex]);
        buckets[bucketIndex].insert(key, value);
    }

    // Find value by key
    bool find(const Key& key, Value& value) {
        size_t bucketIndex = hashFunc(key) % numBuckets;
        std::lock_guard<std::mutex> lock(mutexes[bucketIndex]);
        return buckets[bucketIndex].find(key, value);
    }

    // Remove key-value pair
    bool remove(const Key& key) {
        size_t bucketIndex = hashFunc(key) % numBuckets;
        std::lock_guard<std::mutex> lock(mutexes[bucketIndex]);
        return buckets[bucketIndex].remove(key);
    }

    // Resize hash index dynamically
    void resize(size_t newNumBuckets) {
        std::vector<HashBucket<Key, Value>> newBuckets(newNumBuckets);
        for (size_t i = 0; i < numBuckets; ++i) {
            for (const auto& entry : buckets[i].bucket) {
                size_t newBucketIndex = hashFunc(entry.first) % newNumBuckets;
                newBuckets[newBucketIndex].insert(entry.first, entry.second);
            }
        }
        buckets.swap(newBuckets);
        numBuckets = newNumBuckets;
    }

private:
    size_t numBuckets;
    std::function<size_t(const Key&)> hashFunc;
    std::vector<HashBucket<Key, Value>> buckets;
    std::vector<std::mutex> mutexes = std::vector<std::mutex>(numBuckets);
};

// Usage
int main() {
    auto hashFunc = [](const int& key) { return std::hash<int>()(key); };

    HashIndex<int, std::string> hashIndex(10, hashFunc);

    // Insert some key-value pairs
    hashIndex.insert(1, "Value1");
    hashIndex.insert(2, "Value2");
    hashIndex.insert(3, "Value3");

    // Find value by key
    std::string value;
    if (hashIndex.find(2, value)) {
        std::cout << "Found key 2 with value: " << value << std::endl;
    } else {
        std::cout << "Key 2 not found" << std::endl;
    }

    // Remove key
    if (hashIndex.remove(3)) {
        std::cout << "Key 3 removed successfully" << std::endl;
    }

    // Resize hash index
    hashIndex.resize(20);

    return 0;
}

// Advanced feature: Thread-safe parallel inserts and lookups
#include <thread>
#include <atomic>

void parallelInsert(HashIndex<int, std::string>& index, const std::vector<int>& keys, const std::vector<std::string>& values) {
    for (size_t i = 0; i < keys.size(); ++i) {
        index.insert(keys[i], values[i]);
    }
}

void parallelLookup(HashIndex<int, std::string>& index, const std::vector<int>& keys) {
    for (const auto& key : keys) {
        std::string value;
        if (index.find(key, value)) {
            std::cout << "Key: " << key << " found with value: " << value << std::endl;
        } else {
            std::cout << "Key: " << key << " not found" << std::endl;
        }
    }
}

int main_parallel() {
    auto hashFunc = [](const int& key) { return std::hash<int>()(key); };
    HashIndex<int, std::string> hashIndex(10, hashFunc);

    std::vector<int> keys1 = {1, 2, 3, 4, 5};
    std::vector<int> keys2 = {6, 7, 8, 9, 10};
    std::vector<std::string> values1 = {"Value1", "Value2", "Value3", "Value4", "Value5"};
    std::vector<std::string> values2 = {"Value6", "Value7", "Value8", "Value9", "Value10"};

    std::thread t1(parallelInsert, std::ref(hashIndex), std::ref(keys1), std::ref(values1));
    std::thread t2(parallelInsert, std::ref(hashIndex), std::ref(keys2), std::ref(values2));

    t1.join();
    t2.join();

    std::thread t3(parallelLookup, std::ref(hashIndex), std::ref(keys1));
    std::thread t4(parallelLookup, std::ref(hashIndex), std::ref(keys2));

    t3.join();
    t4.join();

    return 0;
}