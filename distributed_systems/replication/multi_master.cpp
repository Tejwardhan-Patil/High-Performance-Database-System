#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <random>

// Define the structure for data entries with timestamp for conflict resolution
struct DataEntry {
    std::string value;
    std::time_t timestamp;
};

// Class representing a node in the multi-master replication system
class MasterNode {
private:
    std::map<std::string, DataEntry> dataStore;
    std::mutex dataMutex;
    std::vector<MasterNode*> peers; // List of peer nodes

    // Simulates network delay by sleeping for a random duration
    void simulateNetworkDelay() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(50, 300);
        std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
    }

public:
    // Add a peer node to the replication network
    void addPeer(MasterNode* peer) {
        peers.push_back(peer);
    }

    // Write data to the node and replicate to peers
    void writeData(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(dataMutex);

        // Get the current time as timestamp
        std::time_t currentTime = std::time(nullptr);

        // Store the data with a timestamp
        dataStore[key] = { value, currentTime };
        std::cout << "Data written to local node: " << key << " -> " << value << " at " << std::ctime(&currentTime) << std::endl;

        // Replicate data to all peer nodes
        for (auto& peer : peers) {
            peer->replicateData(key, value, currentTime);
        }
    }

    // Replicate data from other nodes
    void replicateData(const std::string& key, const std::string& value, std::time_t timestamp) {
        std::lock_guard<std::mutex> lock(dataMutex);

        // Simulate network delay
        simulateNetworkDelay();

        // If the key already exists, resolve conflicts using timestamps
        if (dataStore.find(key) != dataStore.end()) {
            if (dataStore[key].timestamp < timestamp) {
                // Update the value if the incoming timestamp is newer
                dataStore[key] = { value, timestamp };
                std::cout << "Data updated from replication: " << key << " -> " << value << " at " << std::ctime(&timestamp) << std::endl;
            } else {
                std::cout << "Data replication ignored due to older timestamp for key: " << key << std::endl;
            }
        } else {
            // Insert the new data if the key does not exist
            dataStore[key] = { value, timestamp };
            std::cout << "Data replicated: " << key << " -> " << value << " at " << std::ctime(&timestamp) << std::endl;
        }
    }

    // Read data from the node
    std::string readData(const std::string& key) {
        std::lock_guard<std::mutex> lock(dataMutex);

        if (dataStore.find(key) != dataStore.end()) {
            return dataStore[key].value;
        } else {
            return "Key not found";
        }
    }
};

// Function for simulating client requests to write data to a master node
void clientRequest(MasterNode& master, const std::string& key, const std::string& value) {
    master.writeData(key, value);
}

// Function for simulating a read request from a master node
void readRequest(MasterNode& master, const std::string& key) {
    std::string value = master.readData(key);
    std::cout << "Read data from node: " << key << " -> " << value << std::endl;
}

int main() {
    // Create master nodes
    MasterNode master1, master2, master3;

    // Add peer nodes for replication
    master1.addPeer(&master2);
    master1.addPeer(&master3);
    master2.addPeer(&master1);
    master2.addPeer(&master3);
    master3.addPeer(&master1);
    master3.addPeer(&master2);

    // Simulate client write requests
    std::thread t1(clientRequest, std::ref(master1), "key1", "value1");
    std::thread t2(clientRequest, std::ref(master2), "key2", "value2");
    std::thread t3(clientRequest, std::ref(master3), "key1", "new_value1"); // Conflict on key1

    // Wait for all client requests to finish
    t1.join();
    t2.join();
    t3.join();

    // Simulate read requests
    std::thread t4(readRequest, std::ref(master1), "key1");
    std::thread t5(readRequest, std::ref(master2), "key2");
    std::thread t6(readRequest, std::ref(master3), "key1");

    // Wait for all read requests to finish
    t4.join();
    t5.join();
    t6.join();

    return 0;
}