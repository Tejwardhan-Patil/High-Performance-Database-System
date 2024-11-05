#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <functional>
#include <chrono>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <atomic>

using namespace std;

class Node {
public:
    string id;
    Node(string nodeId) : id(nodeId) {}
};

class ConsistentHashing {
private:
    map<size_t, Node*> hashRing;
    size_t numReplicas;
    hash<string> hashFunction;

public:
    ConsistentHashing(size_t replicas = 100) : numReplicas(replicas) {}

    size_t hashKey(const string &key) {
        return hashFunction(key);
    }

    void addNode(Node* node) {
        for (size_t i = 0; i < numReplicas; ++i) {
            size_t hash = hashKey(node->id + to_string(i));
            hashRing[hash] = node;
        }
    }

    void removeNode(Node* node) {
        for (size_t i = 0; i < numReplicas; ++i) {
            size_t hash = hashKey(node->id + to_string(i));
            hashRing.erase(hash);
        }
    }

    Node* getNode(const string &key) {
        if (hashRing.empty()) return nullptr;

        size_t hash = hashKey(key);
        auto it = hashRing.lower_bound(hash);

        if (it == hashRing.end()) {
            return hashRing.begin()->second;
        }
        return it->second;
    }

    void displayNodes() {
        for (const auto& entry : hashRing) {
            cout << "Hash: " << entry.first << ", Node ID: " << entry.second->id << endl;
        }
    }
};

class RoundRobinBalancer {
private:
    vector<Node*> nodes;
    atomic<int> currentIndex;

public:
    RoundRobinBalancer() : currentIndex(0) {}

    void addNode(Node* node) {
        nodes.push_back(node);
    }

    void removeNode(const string &id) {
        nodes.erase(remove_if(nodes.begin(), nodes.end(), [&](Node* node) {
            return node->id == id;
        }), nodes.end());
    }

    Node* getNextNode() {
        if (nodes.empty()) return nullptr;
        int idx = currentIndex.fetch_add(1) % nodes.size();
        return nodes[idx];
    }
};

class LoadBalancer {
private:
    ConsistentHashing consistentHasher;
    RoundRobinBalancer roundRobinBalancer;
    mutex mtx;

public:
    LoadBalancer() : consistentHasher(100) {}

    void addNode(const string &id) {
        lock_guard<mutex> lock(mtx);
        Node* node = new Node(id);
        consistentHasher.addNode(node);
        roundRobinBalancer.addNode(node);
        cout << "Node " << id << " added to the system." << endl;
    }

    void removeNode(const string &id) {
        lock_guard<mutex> lock(mtx);
        consistentHasher.removeNode(new Node(id));
        roundRobinBalancer.removeNode(id);
        cout << "Node " << id << " removed from the system." << endl;
    }

    Node* getNodeForKey(const string &key) {
        return consistentHasher.getNode(key);
    }

    Node* getRoundRobinNode() {
        return roundRobinBalancer.getNextNode();
    }
};

class ClientRequest {
public:
    string key;
    ClientRequest(const string &key) : key(key) {}
};

class Server {
private:
    LoadBalancer* loadBalancer;

    void handleRequest(ClientRequest request) {
        Node* node = loadBalancer->getNodeForKey(request.key);
        if (node) {
            cout << "Request for key: " << request.key << " handled by node: " << node->id << endl;
        } else {
            cout << "No available nodes to handle the request." << endl;
        }
    }

public:
    Server(LoadBalancer* balancer) : loadBalancer(balancer) {}

    void receiveRequest(ClientRequest request) {
        thread t(&Server::handleRequest, this, request);
        t.join();
    }
};

void simulateClientRequests(Server &server, int requestCount) {
    vector<string> keys = { "apple", "banana", "cherry", "date", "elderberry" };
    for (int i = 0; i < requestCount; ++i) {
        string key = keys[rand() % keys.size()];
        ClientRequest request(key);
        server.receiveRequest(request);
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

int main() {
    LoadBalancer loadBalancer;

    // Add nodes to the system
    loadBalancer.addNode("Node1");
    loadBalancer.addNode("Node2");
    loadBalancer.addNode("Node3");

    Server server(&loadBalancer);

    // Simulate client requests
    thread client1(simulateClientRequests, ref(server), 10);
    thread client2(simulateClientRequests, ref(server), 10);

    client1.join();
    client2.join();

    // Remove a node
    loadBalancer.removeNode("Node2");

    // Simulate more client requests
    thread client3(simulateClientRequests, ref(server), 5);
    client3.join();

    return 0;
}