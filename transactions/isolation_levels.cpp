#include <iostream>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>

enum class IsolationLevel {
    READ_UNCOMMITTED,
    READ_COMMITTED,
    REPEATABLE_READ,
    SERIALIZABLE
};

class Transaction {
public:
    int transaction_id;
    IsolationLevel isolation_level;
    bool active;
    std::unordered_map<int, int> local_data;

    Transaction(int id, IsolationLevel level) : transaction_id(id), isolation_level(level), active(true) {}

    void read(int key, int& value, const std::unordered_map<int, int>& global_data) {
        if (isolation_level == IsolationLevel::READ_UNCOMMITTED) {
            value = global_data.at(key);  // No checks, direct access
        } else if (isolation_level == IsolationLevel::READ_COMMITTED) {
            std::lock_guard<std::mutex> lock(data_mutex);
            value = global_data.at(key);  // Ensures committed data
        } else if (isolation_level == IsolationLevel::REPEATABLE_READ) {
            if (local_data.find(key) != local_data.end()) {
                value = local_data[key];  // Reads from local snapshot
            } else {
                std::lock_guard<std::mutex> lock(data_mutex);
                value = global_data.at(key);
                local_data[key] = value;
            }
        } else if (isolation_level == IsolationLevel::SERIALIZABLE) {
            std::lock_guard<std::mutex> lock(data_mutex);
            if (local_data.find(key) != local_data.end()) {
                value = local_data[key];
            } else {
                value = global_data.at(key);
                local_data[key] = value;
            }
        }
    }

    void write(int key, int value, std::unordered_map<int, int>& global_data) {
        if (isolation_level == IsolationLevel::READ_UNCOMMITTED || isolation_level == IsolationLevel::READ_COMMITTED) {
            std::lock_guard<std::mutex> lock(data_mutex);
            global_data[key] = value;
        } else {
            local_data[key] = value;
        }
    }

    void commit(std::unordered_map<int, int>& global_data) {
        std::lock_guard<std::mutex> lock(data_mutex);
        for (const auto& pair : local_data) {
            global_data[pair.first] = pair.second;
        }
        active = false;
    }

    void rollback() {
        local_data.clear();
        active = false;
    }

    bool is_active() {
        return active;
    }

private:
    static std::mutex data_mutex;
};

std::mutex Transaction::data_mutex;

class Database {
public:
    std::unordered_map<int, int> data;
    std::vector<Transaction> active_transactions;
    std::mutex db_mutex;

    Database() {
        // Initial database setup
        data[1] = 100;
        data[2] = 200;
        data[3] = 300;
    }

    void begin_transaction(int transaction_id, IsolationLevel isolation_level) {
        std::lock_guard<std::mutex> lock(db_mutex);
        active_transactions.emplace_back(transaction_id, isolation_level);
    }

    void read(int transaction_id, int key, int& value) {
        Transaction& txn = get_transaction(transaction_id);
        txn.read(key, value, data);
    }

    void write(int transaction_id, int key, int value) {
        Transaction& txn = get_transaction(transaction_id);
        txn.write(key, value, data);
    }

    void commit_transaction(int transaction_id) {
        Transaction& txn = get_transaction(transaction_id);
        txn.commit(data);
        remove_transaction(transaction_id);
    }

    void rollback_transaction(int transaction_id) {
        Transaction& txn = get_transaction(transaction_id);
        txn.rollback();
        remove_transaction(transaction_id);
    }

private:
    Transaction& get_transaction(int transaction_id) {
        for (auto& txn : active_transactions) {
            if (txn.transaction_id == transaction_id) {
                return txn;
            }
        }
        throw std::runtime_error("Transaction not found");
    }

    void remove_transaction(int transaction_id) {
        std::lock_guard<std::mutex> lock(db_mutex);
        active_transactions.erase(
            std::remove_if(active_transactions.begin(), active_transactions.end(),
                           [transaction_id](Transaction& txn) { return txn.transaction_id == transaction_id; }),
            active_transactions.end());
    }
};

void simulate_transactions(Database& db) {
    db.begin_transaction(1, IsolationLevel::READ_COMMITTED);
    int value;
    db.read(1, 1, value);
    std::cout << "Transaction 1 Read: " << value << std::endl;
    db.write(1, 1, 150);
    db.commit_transaction(1);

    db.begin_transaction(2, IsolationLevel::REPEATABLE_READ);
    db.read(2, 1, value);
    std::cout << "Transaction 2 Read: " << value << std::endl;
    db.write(2, 2, 250);
    db.commit_transaction(2);

    db.begin_transaction(3, IsolationLevel::SERIALIZABLE);
    db.read(3, 3, value);
    std::cout << "Transaction 3 Read: " << value << std::endl;
    db.write(3, 3, 350);
    db.commit_transaction(3);
}

int main() {
    Database db;
    simulate_transactions(db);

    return 0;
}