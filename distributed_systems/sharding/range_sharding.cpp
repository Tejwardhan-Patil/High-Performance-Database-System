#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <string>

class RangeShard {
public:
    RangeShard(int min_range, int max_range)
        : min_range(min_range), max_range(max_range) {}

    bool can_accept(int key) const {
        return key >= min_range && key <= max_range;
    }

    void insert(int key, const std::string& value) {
        storage[key] = value;
    }

    std::string retrieve(int key) const {
        auto it = storage.find(key);
        if (it != storage.end()) {
            return it->second;
        } else {
            return "Key not found";
        }
    }

    void remove(int key) {
        storage.erase(key);
    }

    int get_min_range() const {
        return min_range;
    }

    int get_max_range() const {
        return max_range;
    }

private:
    int min_range;
    int max_range;
    std::map<int, std::string> storage;
};

class ShardManager {
public:
    void add_shard(int min_range, int max_range) {
        shards.push_back(RangeShard(min_range, max_range));
    }

    void insert(int key, const std::string& value) {
        for (auto& shard : shards) {  
            if (shard.can_accept(key)) {
                shard.insert(key, value);
                return;
            }
        }
        std::cerr << "No shard found for key: " << key << std::endl;
    }

    std::string retrieve(int key) {
        for (const auto& shard : shards) {
            if (shard.can_accept(key)) {
                return shard.retrieve(key);
            }
        }
        return "No shard found for key.";
    }

    void remove(int key) {
        for (auto& shard : shards) {  
            if (shard.can_accept(key)) {
                shard.remove(key);
                return;
            }
        }
        std::cerr << "No shard found for key: " << key << std::endl;
    }

private:
    std::vector<RangeShard> shards;
};

int main() {
    ShardManager shard_manager;

    // Add shards
    shard_manager.add_shard(0, 100);
    shard_manager.add_shard(101, 200);
    shard_manager.add_shard(201, 300);

    // Insert values
    shard_manager.insert(50, "Value for key 50");
    shard_manager.insert(150, "Value for key 150");
    shard_manager.insert(250, "Value for key 250");

    // Retrieve values
    std::cout << "Retrieve key 50: " << shard_manager.retrieve(50) << std::endl;
    std::cout << "Retrieve key 150: " << shard_manager.retrieve(150) << std::endl;
    std::cout << "Retrieve key 250: " << shard_manager.retrieve(250) << std::endl;

    // Remove values
    shard_manager.remove(50);
    std::cout << "Retrieve key 50 after removal: " << shard_manager.retrieve(50) << std::endl;

    return 0;
}