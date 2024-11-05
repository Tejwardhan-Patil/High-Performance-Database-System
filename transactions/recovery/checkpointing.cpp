#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>

std::mutex checkpoint_mutex;

// Represents a transaction with a unique ID and its state
enum class TransactionState {
    ACTIVE,
    COMMITTED,
    ABORTED
};

class Transaction {
public:
    int id;
    TransactionState state;
    std::vector<std::string> operations;

    Transaction(int tid) : id(tid), state(TransactionState::ACTIVE) {}

    void add_operation(const std::string &operation) {
        operations.push_back(operation);
    }

    void commit() {
        state = TransactionState::COMMITTED;
    }

    void abort() {
        state = TransactionState::ABORTED;
    }
};

// Write-Ahead Log (WAL) for storing transaction operations
class WriteAheadLog {
public:
    std::ofstream log_file;

    WriteAheadLog(const std::string &filename) {
        log_file.open(filename, std::ios::app);
        if (!log_file.is_open()) {
            throw std::runtime_error("Failed to open WAL file.");
        }
    }

    ~WriteAheadLog() {
        if (log_file.is_open()) {
            log_file.close();
        }
    }

    void log_transaction(const Transaction &txn) {
        for (const auto &op : txn.operations) {
            log_file << "Transaction " << txn.id << ": " << op << "\n";
        }
    }

    void log_commit(int txn_id) {
        log_file << "Transaction " << txn_id << " committed.\n";
    }

    void log_abort(int txn_id) {
        log_file << "Transaction " << txn_id << " aborted.\n";
    }
};

// Checkpoint manager for creating checkpoints to reduce recovery time
class CheckpointManager {
public:
    std::ofstream checkpoint_file;

    CheckpointManager(const std::string &filename) {
        checkpoint_file.open(filename, std::ios::app);
        if (!checkpoint_file.is_open()) {
            throw std::runtime_error("Failed to open checkpoint file.");
        }
    }

    ~CheckpointManager() {
        if (checkpoint_file.is_open()) {
            checkpoint_file.close();
        }
    }

    void create_checkpoint(const std::unordered_map<int, Transaction> &transactions) {
        std::lock_guard<std::mutex> lock(checkpoint_mutex);
        checkpoint_file << "Checkpoint begin\n";
        for (const auto &pair : transactions) {
            const Transaction &txn = pair.second;
            checkpoint_file << "Transaction " << txn.id << " state: " << (txn.state == TransactionState::COMMITTED ? "committed" : "active") << "\n";
        }
        checkpoint_file << "Checkpoint end\n";
    }
};

// Recovery manager for restoring the database to a consistent state after a failure
class RecoveryManager {
public:
    void recover_from_log(const std::string &log_filename, std::unordered_map<int, Transaction> &transactions) {
        std::ifstream log_file(log_filename);
        if (!log_file.is_open()) {
            throw std::runtime_error("Failed to open log file.");
        }

        std::string line;
        while (std::getline(log_file, line)) {
            if (line.find("committed") != std::string::npos) {
                int txn_id = extract_transaction_id(line);
                if (transactions.find(txn_id) != transactions.end()) {
                    transactions[txn_id].commit();
                }
            }
            else if (line.find("aborted") != std::string::npos) {
                int txn_id = extract_transaction_id(line);
                if (transactions.find(txn_id) != transactions.end()) {
                    transactions[txn_id].abort();
                }
            }
        }

        log_file.close();
    }

private:
    int extract_transaction_id(const std::string &line) {
        size_t pos = line.find("Transaction ");
        return std::stoi(line.substr(pos + 12, line.find(":") - pos - 12));
    }
};

// Main transaction manager to handle transactions and coordinate with logging and recovery
class TransactionManager {
public:
    TransactionManager(const std::string &wal_file, const std::string &checkpoint_file)
        : wal(wal_file), checkpoint_manager(checkpoint_file) {}

    Transaction &begin_transaction() {
        int tid = ++last_transaction_id;
        Transaction txn(tid);
        transactions[tid] = txn;
        return transactions[tid];
    }

    void commit_transaction(int txn_id) {
        if (transactions.find(txn_id) != transactions.end()) {
            transactions[txn_id].commit();
            wal.log_transaction(transactions[txn_id]);
            wal.log_commit(txn_id);
        }
    }

    void abort_transaction(int txn_id) {
        if (transactions.find(txn_id) != transactions.end()) {
            transactions[txn_id].abort();
            wal.log_abort(txn_id);
        }
    }

    void create_checkpoint() {
        checkpoint_manager.create_checkpoint(transactions);
    }

    void recover() {
        RecoveryManager recovery_manager;
        recovery_manager.recover_from_log("wal.log", transactions);
    }

private:
    int last_transaction_id = 0;
    std::unordered_map<int, Transaction> transactions;
    WriteAheadLog wal;
    CheckpointManager checkpoint_manager;
};

void simulate_transactions(TransactionManager &tm) {
    Transaction &txn1 = tm.begin_transaction();
    txn1.add_operation("UPDATE account SET balance = balance - 100 WHERE id = 1");
    txn1.add_operation("UPDATE account SET balance = balance + 100 WHERE id = 2");

    Transaction &txn2 = tm.begin_transaction();
    txn2.add_operation("DELETE FROM account WHERE id = 3");

    tm.commit_transaction(txn1.id);
    tm.abort_transaction(txn2.id);

    tm.create_checkpoint();
}

int main() {
    TransactionManager transaction_manager("wal.log", "checkpoint.log");

    simulate_transactions(transaction_manager);

    std::cout << "Recovery process initiated...\n";
    transaction_manager.recover();
    std::cout << "Recovery completed.\n";

    return 0;
}