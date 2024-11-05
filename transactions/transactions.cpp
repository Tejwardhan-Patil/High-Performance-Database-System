#include <iostream>
#include <unordered_map>
#include <vector>
#include <mutex>
#include "write_ahead_log.h"  
#include "concurrency_control.h"  
#include "isolation_levels.h"  
#include "recovery.h"  

enum TransactionState {
    ACTIVE,
    COMMITTED,
    ABORTED
};

class Transaction {
public:
    int transaction_id;
    TransactionState state;
    std::vector<std::string> log_entries;  // Stores log entries for the transaction
    IsolationLevel isolation_level;

    Transaction(int id, IsolationLevel level)
        : transaction_id(id), state(ACTIVE), isolation_level(level) {}

    void addLogEntry(const std::string& entry) {
        log_entries.push_back(entry);
    }
};

class TransactionManager {
private:
    std::unordered_map<int, Transaction*> active_transactions;
    std::mutex tx_mutex;
    WriteAheadLog wal;  // Write-ahead log instance for logging
    ConcurrencyControl cc;  // Concurrency control for handling locks, timestamps, etc
    RecoveryManager recovery_manager;  // Handles crash recovery

public:
    TransactionManager() {
        recovery_manager.recover();  // Perform recovery during startup
    }

    ~TransactionManager() {
        for (auto& entry : active_transactions) {
            delete entry.second;
        }
    }

    Transaction* beginTransaction(IsolationLevel isolation_level) {
        std::lock_guard<std::mutex> lock(tx_mutex);
        int tx_id = generateTransactionId();
        Transaction* tx = new Transaction(tx_id, isolation_level);
        active_transactions[tx_id] = tx;

        wal.logBegin(tx_id);  // Log the start of the transaction

        return tx;
    }

    void commitTransaction(int tx_id) {
        std::lock_guard<std::mutex> lock(tx_mutex);
        if (active_transactions.find(tx_id) == active_transactions.end()) {
            std::cerr << "Transaction not found\n";
            return;
        }

        Transaction* tx = active_transactions[tx_id];

        // Perform the commit sequence
        if (cc.commit(tx)) {
            wal.logCommit(tx_id);  // Log the commit
            tx->state = COMMITTED;
            std::cout << "Transaction " << tx_id << " committed successfully\n";
        } else {
            std::cerr << "Commit failed for transaction " << tx_id << "\n";
            abortTransaction(tx_id);
        }

        cleanupTransaction(tx_id);
    }

    void abortTransaction(int tx_id) {
        std::lock_guard<std::mutex> lock(tx_mutex);
        if (active_transactions.find(tx_id) == active_transactions.end()) {
            std::cerr << "Transaction not found\n";
            return;
        }

        Transaction* tx = active_transactions[tx_id];
        wal.logAbort(tx_id);  // Log the abort
        cc.rollback(tx);  // Rollback any changes made by the transaction
        tx->state = ABORTED;
        std::cout << "Transaction " << tx_id << " aborted successfully\n";

        cleanupTransaction(tx_id);
    }

    void rollbackTransaction(int tx_id) {
        std::lock_guard<std::mutex> lock(tx_mutex);
        if (active_transactions.find(tx_id) == active_transactions.end()) {
            std::cerr << "Transaction not found\n";
            return;
        }

        Transaction* tx = active_transactions[tx_id];
        wal.logRollback(tx_id);  // Log the rollback
        cc.rollback(tx);  // Rollback any changes made by the transaction
        std::cout << "Transaction " << tx_id << " rolled back successfully\n";
    }

private:
    int generateTransactionId() {
        static int next_id = 0;
        return ++next_id;
    }

    void cleanupTransaction(int tx_id) {
        delete active_transactions[tx_id];
        active_transactions.erase(tx_id);
    }
};

// ConcurrencyControl class to simulate 2PL or TO mechanisms
class ConcurrencyControl {
private:
    std::set<int> locked_resources;

    bool lockResource(int resource) {
        if (locked_resources.find(resource) != locked_resources.end()) {
            return false; // Resource is already locked
        }
        locked_resources.insert(resource);
        return true;
    }

    void releaseLocks(Transaction* tx) {
        for (int resource : tx->write_set) {
            locked_resources.erase(resource);
        }
    }

public:
    bool commit(Transaction* tx) {
        // Try to acquire locks for all resources in the write set
        for (int resource : tx->write_set) {
            if (!lockResource(resource)) {
                rollback(tx);
                return false;
            }
        }
        // Commit is successful, release locks
        releaseLocks(tx);
        return true;
    }

    void rollback(Transaction* tx) {
        std::cout << "Rolling back transaction " << tx->transaction_id << "\n";
        releaseLocks(tx);
    }
};

// WriteAheadLog class for logging purposes
class WriteAheadLog {
public:
    void logBegin(int tx_id) {
        std::cout << "Logging start of transaction " << tx_id << "\n";
    }

    void logCommit(int tx_id) {
        std::cout << "Logging commit of transaction " << tx_id << "\n";
    }

    void logAbort(int tx_id) {
        std::cout << "Logging abort of transaction " << tx_id << "\n";
    }

    void logRollback(int tx_id) {
        std::cout << "Logging rollback of transaction " << tx_id << "\n";
    }
};

// RecoveryManager class for recovery during startup
class RecoveryManager {
public:
    void recover() {
        std::cout << "Recovering from previous failure, replaying logs...\n";
        // Replay logs or perform checkpointing to recover the system state
    }
};

// IsolationLevel enum
enum IsolationLevel {
    READ_UNCOMMITTED,
    READ_COMMITTED,
    REPEATABLE_READ,
    SERIALIZABLE
};

int main() {
    TransactionManager tx_manager;

    // Start a transaction with REPEATABLE_READ isolation level
    Transaction* tx1 = tx_manager.beginTransaction(REPEATABLE_READ);

    // Perform some operations on the transaction
    tx1->addLogEntry("Operation 1");
    tx1->addLogEntry("Operation 2");

    // Commit the transaction
    tx_manager.commitTransaction(tx1->transaction_id);

    // Start a new transaction
    Transaction* tx2 = tx_manager.beginTransaction(SERIALIZABLE);
    tx2->addLogEntry("Operation A");

    // Abort the transaction
    tx_manager.abortTransaction(tx2->transaction_id);

    return 0;
}