#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <chrono>

class Server {
public:
    std::string name;
    int id;
    Server(int id, std::string name) : id(id), name(name) {}
    
    void processRequest(int requestId) {
        std::cout << "Server " << name << " processing request " << requestId << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Simulating processing time
    }
};

class LoadBalancer {
private:
    std::vector<Server> servers;
    int currentIndex;
    std::mutex mtx;
    
public:
    LoadBalancer(std::vector<Server> servers) : servers(servers), currentIndex(0) {}
    
    Server getNextServer() {
        std::lock_guard<std::mutex> lock(mtx);
        Server server = servers[currentIndex];
        currentIndex = (currentIndex + 1) % servers.size();
        return server;
    }
};

class RequestQueue {
private:
    std::queue<int> requestQueue;
    std::mutex mtx;
    std::condition_variable cv;
    bool stopped;

public:
    RequestQueue() : stopped(false) {}

    void addRequest(int requestId) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            requestQueue.push(requestId);
        }
        cv.notify_one();
    }

    int getNextRequest() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !requestQueue.empty() || stopped; });

        if (stopped && requestQueue.empty()) {
            return -1;  // Indicating the queue has stopped
        }

        int requestId = requestQueue.front();
        requestQueue.pop();
        return requestId;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            stopped = true;
        }
        cv.notify_all();
    }
};

void handleRequests(LoadBalancer& lb, RequestQueue& requestQueue) {
    while (true) {
        int requestId = requestQueue.getNextRequest();
        if (requestId == -1) break;

        Server server = lb.getNextServer();
        server.processRequest(requestId);
    }
}

int main() {
    // Create servers
    std::vector<Server> servers = {
        Server(1, "Server1"),
        Server(2, "Server2"),
        Server(3, "Server3")
    };

    // Create load balancer
    LoadBalancer lb(servers);

    // Create request queue
    RequestQueue requestQueue;

    // Create a pool of threads to handle requests
    std::vector<std::thread> threadPool;
    for (int i = 0; i < 3; ++i) {
        threadPool.push_back(std::thread(handleRequests, std::ref(lb), std::ref(requestQueue)));
    }

    // Simulate incoming requests
    for (int i = 1; i <= 10; ++i) {
        requestQueue.addRequest(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));  // Simulating request intervals
    }

    // Stop the request queue and wait for threads to finish
    requestQueue.stop();
    for (auto& thread : threadPool) {
        thread.join();
    }

    std::cout << "All requests processed." << std::endl;

    return 0;
}