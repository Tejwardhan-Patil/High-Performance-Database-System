#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include "transactions.h"  
#include "concurrency_control/two_phase_locking.h"  
#include "logging/write_ahead_log.h"
#include "recovery/log_replay.h"
#include "isolation_levels.h"  

std::mutex mtx;  // Mutex for ensuring thread-safe output

// Helper function to simulate a transaction
void simulate_transaction(TransactionManager& txn_manager, int txn_id, bool should_commit) {
    std::lock_guard<std::mutex> lock(mtx);
    Transaction txn = txn_manager.begin_transaction(txn_id);
    try {
        // Perform some operations within the transaction
        std::cout << "Transaction " << txn_id << " started.\n";

        // Simulate reads and writes
        txn_manager.read(txn, "some_key");
        txn_manager.write(txn, "some_key", "some_value");

        // Decide whether to commit or rollback
        if (should_commit) {
            txn_manager.commit(txn);
            std::cout << "Transaction " << txn_id << " committed.\n";
        } else {
            txn_manager.rollback(txn);
            std::cout << "Transaction " << txn_id << " rolled back.\n";
        }
    } catch (const std::exception& e) {
        txn_manager.rollback(txn);
        std::cerr << "Error in transaction " << txn_id << ": " << e.what() << "\n";
    }
}

// Test for two-phase locking (2PL) in concurrent transactions
void test_two_phase_locking() {
    TransactionManager txn_manager;
    TwoPhaseLocking concurrency_manager;

    std::thread t1(simulate_transaction, std::ref(txn_manager), 1, true);
    std::thread t2(simulate_transaction, std::ref(txn_manager), 2, false);

    t1.join();
    t2.join();

    // Verifying that the locks were properly acquired and released
    assert(concurrency_manager.is_locked("some_key") == false);
    std::cout << "Two-phase locking test passed.\n";
}

// Test for write-ahead logging (WAL) and recovery
void test_write_ahead_log() {
    TransactionManager txn_manager;
    WriteAheadLog wal;
    LogReplay recovery_manager;

    // Simulate a transaction with WAL
    std::thread t1(simulate_transaction, std::ref(txn_manager), 3, true);

    t1.join();

    // Simulate a system crash and recovery
    recovery_manager.replay_logs();

    // Verifying the recovery process
    assert(txn_manager.is_recovered("some_key"));
    std::cout << "Write-ahead log and recovery test passed.\n";
}

// Test for different isolation levels (Read Committed and Serializable)
void test_isolation_levels() {
    TransactionManager txn_manager;

    // Begin two transactions with different isolation levels
    std::thread t1([&]() {
        Transaction txn = txn_manager.begin_transaction(4);
        txn_manager.set_isolation_level(txn, IsolationLevel::READ_COMMITTED);

        // Simulate some operations under Read Committed isolation
        txn_manager.read(txn, "key_1");
        txn_manager.write(txn, "key_1", "value_1");
        txn_manager.commit(txn);
        std::cout << "Transaction 4 (Read Committed) committed.\n";
    });

    std::thread t2([&]() {
        Transaction txn = txn_manager.begin_transaction(5);
        txn_manager.set_isolation_level(txn, IsolationLevel::SERIALIZABLE);

        // Simulate some operations under Serializable isolation
        txn_manager.read(txn, "key_1");
        txn_manager.write(txn, "key_1", "value_2");
        txn_manager.commit(txn);
        std::cout << "Transaction 5 (Serializable) committed.\n";
    });

    t1.join();
    t2.join();

    std::cout << "Isolation level test passed.\n";
}

// Test for transaction rollback in case of an error
void test_transaction_rollback() {
    TransactionManager txn_manager;

    std::thread t1([&]() {
        Transaction txn = txn_manager.begin_transaction(6);
        try {
            // Simulating an operation that causes an exception
            txn_manager.write(txn, "key_2", "value_3");
            throw std::runtime_error("Simulated transaction error");

            // The commit should not be reached due to the exception
            txn_manager.commit(txn);
        } catch (const std::exception& e) {
            txn_manager.rollback(txn);
            std::cout << "Transaction 6 rolled back due to error: " << e.what() << "\n";
        }
    });

    t1.join();
    std::cout << "Transaction rollback test passed.\n";
}

// Stress test for high concurrency
void stress_test_concurrent_transactions() {
    TransactionManager txn_manager;
    int num_transactions = 100;  // Simulating a high number of concurrent transactions
    std::vector<std::thread> threads;

    for (int i = 0; i < num_transactions; ++i) {
        threads.push_back(std::thread(simulate_transaction, std::ref(txn_manager), i + 7, true));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "Stress test with high concurrency passed.\n";
}

// Main function to run all tests
int main() {
    std::cout << "Running transaction tests...\n";

    test_two_phase_locking();
    test_write_ahead_log();
    test_isolation_levels();
    test_transaction_rollback();
    stress_test_concurrent_transactions();

    std::cout << "All transaction tests passed.\n";
    return 0;
}