#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <chrono>

// Data structure to hold the replicated data
class DataStore {
public:
    std::map<int, std::string> data;
    std::mutex data_mutex;

    void write(int key, const std::string& value) {
        std::lock_guard<std::mutex> lock(data_mutex);
        data[key] = value;
    }

    std::string read(int key) {
        std::lock_guard<std::mutex> lock(data_mutex);
        return data[key];
    }
};

// Message structure for replication
struct ReplicationMessage {
    int key;
    std::string value;
};

// Queue for replication messages
class ReplicationQueue {
private:
    std::queue<ReplicationMessage> messages;
    std::mutex queue_mutex;
    std::condition_variable cv;

public:
    void push(const ReplicationMessage& msg) {
        std::lock_guard<std::mutex> lock(queue_mutex);
        messages.push(msg);
        cv.notify_all();
    }

    ReplicationMessage pop() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        cv.wait(lock, [this] { return !messages.empty(); });
        ReplicationMessage msg = messages.front();
        messages.pop();
        return msg;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(queue_mutex);
        return messages.empty();
    }
};

// Slave node class to handle replicated data
class SlaveNode {
private:
    DataStore datastore;
    ReplicationQueue& replicationQueue;
    bool running;

    void replicate() {
        while (running) {
            if (!replicationQueue.empty()) {
                ReplicationMessage msg = replicationQueue.pop();
                datastore.write(msg.key, msg.value);
                std::cout << "Slave replicated key: " << msg.key << ", value: " << msg.value << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

public:
    SlaveNode(ReplicationQueue& queue) : replicationQueue(queue), running(true) {}

    void start() {
        std::thread t(&SlaveNode::replicate, this);
        t.detach();
    }

    void stop() {
        running = false;
    }

    std::string read(int key) {
        return datastore.read(key);
    }
};

// Master node class to handle write requests and replication to slaves
class MasterNode {
private:
    DataStore datastore;
    std::vector<SlaveNode*> slaves;
    ReplicationQueue replicationQueue;

public:
    MasterNode(int numSlaves) {
        for (int i = 0; i < numSlaves; ++i) {
            slaves.push_back(new SlaveNode(replicationQueue));
        }

        for (auto& slave : slaves) {
            slave->start();
        }
    }

    ~MasterNode() {
        for (auto& slave : slaves) {
            slave->stop();
            delete slave;
        }
    }

    void write(int key, const std::string& value) {
        datastore.write(key, value);
        std::cout << "Master wrote key: " << key << ", value: " << value << std::endl;

        ReplicationMessage msg = { key, value };
        replicationQueue.push(msg);
    }

    std::string read(int key) {
        return datastore.read(key);
    }

    void stopSlaves() {
        for (auto& slave : slaves) {
            slave->stop();
        }
    }

    // Getter method to access slaves vector
    SlaveNode* getSlave(int index) {
        if (index >= 0 && index < slaves.size()) {
            return slaves[index];
        }
        return nullptr; // Return null if the index is invalid
    }
};

// Simulating a client that interacts with the system
void clientWrite(MasterNode& master, int key, const std::string& value) {
    master.write(key, value);
}

void clientRead(MasterNode& master, int key) {
    std::string value = master.read(key);
    std::cout << "Client read from master: key = " << key << ", value = " << value << std::endl;
}

void clientReadSlave(SlaveNode& slave, int key) {
    std::string value = slave.read(key);
    std::cout << "Client read from slave: key = " << key << ", value = " << value << std::endl;
}

int main() {
    MasterNode master(3); // Create a master with 3 slave nodes

    // Simulate client writes
    std::thread t1(clientWrite, std::ref(master), 1, "value1");
    std::thread t2(clientWrite, std::ref(master), 2, "value2");
    std::thread t3(clientWrite, std::ref(master), 3, "value3");

    t1.join();
    t2.join();
    t3.join();

    // Give some time for replication
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Simulate client reads from master
    clientRead(master, 1);
    clientRead(master, 2);
    clientRead(master, 3);

    // Simulate client reads from one of the slaves
    SlaveNode* slave = master.getSlave(0);
    if (slave) {
        clientReadSlave(*slave, 1);
        clientReadSlave(*slave, 2);
        clientReadSlave(*slave, 3);
    } else {
        std::cerr << "Invalid slave index!" << std::endl;
    }

    master.stopSlaves();
    return 0;
}