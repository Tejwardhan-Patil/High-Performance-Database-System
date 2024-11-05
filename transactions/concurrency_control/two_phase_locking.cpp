#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <chrono>

enum LockType { SHARED, EXCLUSIVE };

class LockManager {
private:
    struct LockRequest {
        int txn_id;
        LockType lock_type;
        std::condition_variable cv;
        bool granted;

        LockRequest(int id, LockType type) : txn_id(id), lock_type(type), granted(false) {}
    };

    std::unordered_map<int, std::vector<LockRequest>> lock_table; // Resource ID -> Lock Requests
    std::mutex mtx;

public:
    void acquireLock(int txn_id, int resource_id, LockType lock_type) {
        std::unique_lock<std::mutex> lock(mtx);

        if (lock_table.find(resource_id) == lock_table.end()) {
            // No current locks on the resource
            lock_table[resource_id].emplace_back(txn_id, lock_type);
            lock_table[resource_id].back().granted = true;
            std::cout << "Transaction " << txn_id << " acquired " 
                      << (lock_type == EXCLUSIVE ? "EXCLUSIVE" : "SHARED") 
                      << " lock on resource " << resource_id << std::endl;
            return;
        }

        // If there are locks, check if the request can be granted
        auto& requests = lock_table[resource_id];
        bool can_grant = true;

        for (const auto& req : requests) {
            if (req.txn_id != txn_id && (req.lock_type == EXCLUSIVE || lock_type == EXCLUSIVE)) {
                can_grant = false;
                break;
            }
        }

        if (can_grant) {
            requests.emplace_back(txn_id, lock_type);
            requests.back().granted = true;
            std::cout << "Transaction " << txn_id << " acquired " 
                      << (lock_type == EXCLUSIVE ? "EXCLUSIVE" : "SHARED") 
                      << " lock on resource " << resource_id << std::endl;
        } else {
            requests.emplace_back(txn_id, lock_type);
            std::cout << "Transaction " << txn_id << " waiting for lock on resource " << resource_id << std::endl;
            requests.back().cv.wait(lock, [&requests, txn_id]() {
                for (auto& req : requests) {
                    if (req.txn_id == txn_id && req.granted) {
                        return true;
                    }
                }
                return false;
            });
        }
    }

    void releaseLock(int txn_id, int resource_id) {
        std::unique_lock<std::mutex> lock(mtx);

        if (lock_table.find(resource_id) == lock_table.end()) {
            std::cout << "No locks found on resource " << resource_id << std::endl;
            return;
        }

        auto& requests = lock_table[resource_id];
        requests.erase(
            std::remove_if(requests.begin(), requests.end(),
                           [txn_id](LockRequest& req) { return req.txn_id == txn_id; }),
            requests.end());

        if (requests.empty()) {
            lock_table.erase(resource_id);
        } else {
            for (auto& req : requests) {
                if (!req.granted) {
                    req.granted = true;
                    req.cv.notify_one();
                    break;
                }
            }
        }

        std::cout << "Transaction " << txn_id << " released lock on resource " << resource_id << std::endl;
    }

    std::unordered_map<int, std::vector<LockRequest>>& getLockTable() {
        return lock_table;
    }
};

class TransactionManager {
private:
    LockManager& lock_manager;
    std::unordered_map<int, std::unordered_set<int>> txn_locks;
    std::unordered_set<int> active_transactions;
    std::mutex mtx;

    // Wait-For Graph: txn_id -> set of transactions it is waiting for
    std::unordered_map<int, std::unordered_set<int>> wait_for_graph;

    bool dfs(int txn_id, std::unordered_set<int>& visited, std::unordered_set<int>& rec_stack) {
        if (rec_stack.find(txn_id) != rec_stack.end()) {
            // Cycle detected (deadlock)
            return true;
        }
        if (visited.find(txn_id) != visited.end()) {
            return false;
        }

        visited.insert(txn_id);
        rec_stack.insert(txn_id);

        if (wait_for_graph.find(txn_id) != wait_for_graph.end()) {
            for (int waiting_for : wait_for_graph[txn_id]) {
                if (dfs(waiting_for, visited, rec_stack)) {
                    return true;
                }
            }
        }

        rec_stack.erase(txn_id);
        return false;
    }

public:
    TransactionManager(LockManager& lm) : lock_manager(lm) {}

    void beginTransaction(int txn_id) {
        std::lock_guard<std::mutex> lock(mtx);
        active_transactions.insert(txn_id);
        std::cout << "Transaction " << txn_id << " started." << std::endl;
    }

    void acquireLock(int txn_id, int resource_id, LockType lock_type) {
        lock_manager.acquireLock(txn_id, resource_id, lock_type);
        std::lock_guard<std::mutex> lock(mtx);
        txn_locks[txn_id].insert(resource_id);

        // Build wait-for graph
        auto& lock_table = lock_manager.getLockTable();
        for (const auto& req : lock_table[resource_id]) {
            if (req.txn_id != txn_id && !req.granted) {
                wait_for_graph[txn_id].insert(req.txn_id);
            }
        }
    }

    void releaseLocks(int txn_id) {
        std::lock_guard<std::mutex> lock(mtx);
        if (txn_locks.find(txn_id) != txn_locks.end()) {
            for (int resource_id : txn_locks[txn_id]) {
                lock_manager.releaseLock(txn_id, resource_id);
                // Remove txn from the wait-for graph
                wait_for_graph.erase(txn_id);
                for (auto& entry : wait_for_graph) {
                    entry.second.erase(txn_id);
                }
            }
            txn_locks.erase(txn_id);
        }
    }

    void commitTransaction(int txn_id) {
        std::lock_guard<std::mutex> lock(mtx);
        if (active_transactions.find(txn_id) != active_transactions.end()) {
            releaseLocks(txn_id);
            active_transactions.erase(txn_id);
            std::cout << "Transaction " << txn_id << " committed." << std::endl;
        }
    }

    void rollbackTransaction(int txn_id) {
        std::lock_guard<std::mutex> lock(mtx);
        if (active_transactions.find(txn_id) != active_transactions.end()) {
            releaseLocks(txn_id);
            active_transactions.erase(txn_id);
            std::cout << "Transaction " << txn_id << " rolled back." << std::endl;
        }
    }

    bool detectDeadlock() {
        std::lock_guard<std::mutex> lock(mtx);
        std::unordered_set<int> visited;
        std::unordered_set<int> rec_stack;

        for (const auto& entry : wait_for_graph) {
            int txn_id = entry.first;
            if (visited.find(txn_id) == visited.end()) {
                if (dfs(txn_id, visited, rec_stack)) {
                    std::cout << "Deadlock detected!" << std::endl;
                    return true;
                }
            }
        }
        std::cout << "No deadlock detected." << std::endl;
        return false;
    }
};

// Simulating two transactions
void transaction1(TransactionManager& tm) {
    tm.beginTransaction(1);
    tm.acquireLock(1, 1, SHARED);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    tm.acquireLock(1, 2, EXCLUSIVE);
    tm.commitTransaction(1);
}

void transaction2(TransactionManager& tm) {
    tm.beginTransaction(2);
    tm.acquireLock(2, 1, EXCLUSIVE);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    tm.acquireLock(2, 2, SHARED);
    tm.commitTransaction(2);
}

int main() {
    LockManager lock_manager;
    TransactionManager txn_manager(lock_manager);

    std::thread t1(transaction1, std::ref(txn_manager));
    std::thread t2(transaction2, std::ref(txn_manager));

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    txn_manager.detectDeadlock();  // Check for deadlock after some time

    t1.join();
    t2.join();

    return 0;
}