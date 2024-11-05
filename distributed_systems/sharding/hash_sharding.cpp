#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <mutex>

// Node class represents a node in the distributed system
class Node {
public:
    std::string nodeId;
    std::map<std::string, std::string> data;

    Node(const std::string& id) : nodeId(id) {}

    void storeData(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(nodeMutex);
        data[key] = value;
    }

    std::string retrieveData(const std::string& key) {
        std::lock_guard<std::mutex> lock(nodeMutex);
        if (data.find(key) != data.end()) {
            return data[key];
        }
        return "Key not found!";
    }

private:
    std::mutex nodeMutex;
};

// Hash function for string-based keys
std::size_t hashFunction(const std::string& key) {
    return std::hash<std::string>{}(key);
}

// ShardingManager class manages the shards (nodes) and distributes data among them
class ShardingManager {
public:
    ShardingManager(const std::vector<std::string>& nodeIds) {
        for (const auto& nodeId : nodeIds) {
            nodes.push_back(std::make_shared<Node>(nodeId));
        }
    }

    // Function to add data into the appropriate node based on hash
    void putData(const std::string& key, const std::string& value) {
        std::size_t nodeIndex = hashFunction(key) % nodes.size();
        nodes[nodeIndex]->storeData(key, value);
    }

    // Function to retrieve data from the appropriate node based on hash
    std::string getData(const std::string& key) {
        std::size_t nodeIndex = hashFunction(key) % nodes.size();
        return nodes[nodeIndex]->retrieveData(key);
    }

    // Function to add new node dynamically
    void addNode(const std::string& nodeId) {
        std::lock_guard<std::mutex> lock(managerMutex);
        nodes.push_back(std::make_shared<Node>(nodeId));
    }

    // Function to remove a node dynamically
    void removeNode(const std::string& nodeId) {
        std::lock_guard<std::mutex> lock(managerMutex);
        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            if ((*it)->nodeId == nodeId) {
                nodes.erase(it);
                break;
            }
        }
    }

private:
    std::vector<std::shared_ptr<Node>> nodes;
    std::mutex managerMutex;
};

// Test the sharding mechanism
void testShardingSystem() {
    std::vector<std::string> nodeIds = { "Node1", "Node2", "Node3" };
    ShardingManager shardManager(nodeIds);

    // Put some data into the system
    shardManager.putData("user123", "Data for user123");
    shardManager.putData("user456", "Data for user456");
    shardManager.putData("user789", "Data for user789");

    // Retrieve data from the system
    std::cout << "Data for user123: " << shardManager.getData("user123") << std::endl;
    std::cout << "Data for user456: " << shardManager.getData("user456") << std::endl;
    std::cout << "Data for user789: " << shardManager.getData("user789") << std::endl;

    // Dynamically add a new node
    shardManager.addNode("Node4");
    shardManager.putData("user101", "Data for user101");
    std::cout << "Data for user101: " << shardManager.getData("user101") << std::endl;

    // Dynamically remove a node
    shardManager.removeNode("Node2");
    shardManager.putData("user202", "Data for user202");
    std::cout << "Data for user202: " << shardManager.getData("user202") << std::endl;
}

int main() {
    testShardingSystem();
    return 0;
}