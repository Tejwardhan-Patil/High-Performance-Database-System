#include <unordered_map>
#include <list>
#include <iostream>

class LFUCache {
private:
    struct Node {
        int key, value, freq;
        Node(int k, int v, int f) : key(k), value(v), freq(f) {}
    };

    int capacity, minFreq;
    std::unordered_map<int, std::list<Node>::iterator> keyNode; // Stores key-node pair
    std::unordered_map<int, std::list<Node>> freqList; // Stores frequency-node list pairs

    void updateFrequency(std::unordered_map<int, std::list<Node>::iterator>::iterator it) {
        auto node = *(it->second);
        freqList[node.freq].erase(it->second);

        if (freqList[node.freq].empty()) {
            freqList.erase(node.freq);
            if (minFreq == node.freq)
                minFreq++;
        }

        node.freq++;
        freqList[node.freq].push_front(node);
        it->second = freqList[node.freq].begin();
    }

public:
    LFUCache(int cap) : capacity(cap), minFreq(0) {}

    int get(int key) {
        if (capacity == 0 || keyNode.find(key) == keyNode.end()) return -1;

        auto it = keyNode.find(key);
        int value = it->second->value;
        updateFrequency(it);
        return value;
    }

    void put(int key, int value) {
        if (capacity == 0) return;

        if (keyNode.find(key) != keyNode.end()) {
            auto it = keyNode.find(key);
            it->second->value = value;
            updateFrequency(it);
        } else {
            if (keyNode.size() == capacity) {
                auto it = freqList[minFreq].back();
                keyNode.erase(it.key);
                freqList[minFreq].pop_back();
                if (freqList[minFreq].empty()) {
                    freqList.erase(minFreq);
                }
            }

            minFreq = 1;
            freqList[minFreq].push_front(Node(key, value, 1));
            keyNode[key] = freqList[minFreq].begin();
        }
    }
};

int main() {
    LFUCache cache(2);

    cache.put(1, 1);
    cache.put(2, 2);
    std::cout << "Get 1: " << cache.get(1) << std::endl; 
    cache.put(3, 3);    // evicts key 2
    std::cout << "Get 2: " << cache.get(2) << std::endl; 
    std::cout << "Get 3: " << cache.get(3) << std::endl; 
    cache.put(4, 4);    // evicts key 1
    std::cout << "Get 1: " << cache.get(1) << std::endl; 
    std::cout << "Get 3: " << cache.get(3) << std::endl; 
    std::cout << "Get 4: " << cache.get(4) << std::endl; 

    return 0;
}