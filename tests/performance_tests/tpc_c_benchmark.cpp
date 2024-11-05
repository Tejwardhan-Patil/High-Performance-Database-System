#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <random>
#include <map>
#include <mutex>
#include "query_executor.h"  
#include "transaction_manager.h" 
#include "connection_pool.h"  

// TPC-C benchmark parameters
constexpr int NUM_WAREHOUSES = 10;
constexpr int NUM_TRANSACTIONS = 100000;
constexpr int NUM_THREADS = 8;
constexpr int MAX_ITEM_ID = 100000;
constexpr int MAX_ORDER_LINE = 10;

// Metrics
std::atomic<int> successful_transactions(0);
std::atomic<int> failed_transactions(0);
std::atomic<long long> total_latency(0);
std::mutex latency_mutex;

struct TransactionResult {
    bool success;
    long long latency;
};

// Random generator
std::random_device rd;
std::mt19937 gen(rd());

int generate_random_int(int min, int max) {
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

// Simulate New Order transaction from TPC-C benchmark
TransactionResult execute_new_order() {
    auto start = std::chrono::high_resolution_clock::now();
    // Start transaction
    TransactionManager tx_manager;
    tx_manager.start_transaction();

    try {
        int warehouse_id = generate_random_int(1, NUM_WAREHOUSES);
        int district_id = generate_random_int(1, 10);
        int customer_id = generate_random_int(1, 3000);
        int num_items = generate_random_int(1, MAX_ORDER_LINE);
        std::map<int, int> item_quantity_map;  // item_id -> quantity

        for (int i = 0; i < num_items; ++i) {
            int item_id = generate_random_int(1, MAX_ITEM_ID);
            int quantity = generate_random_int(1, 10);
            item_quantity_map[item_id] = quantity;
        }

        // Process order in database
        QueryExecutor executor;
        for (const auto& [item_id, quantity] : item_quantity_map) {
            executor.execute_update(
                "UPDATE stock SET quantity = quantity - " + std::to_string(quantity) + 
                " WHERE item_id = " + std::to_string(item_id) + 
                " AND warehouse_id = " + std::to_string(warehouse_id));
        }

        // Commit transaction
        tx_manager.commit_transaction();
        auto end = std::chrono::high_resolution_clock::now();
        long long latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        
        std::lock_guard<std::mutex> lock(latency_mutex);
        total_latency += latency;

        return {true, latency};
    } catch (...) {
        // Rollback transaction on failure
        tx_manager.rollback_transaction();
        return {false, 0};
    }
}

// Worker thread that executes multiple transactions
void worker_thread() {
    for (int i = 0; i < NUM_TRANSACTIONS / NUM_THREADS; ++i) {
        TransactionResult result = execute_new_order();
        if (result.success) {
            successful_transactions++;
        } else {
            failed_transactions++;
        }
    }
}

// Benchmark execution
void run_benchmark() {
    std::cout << "Starting TPC-C Benchmark..." << std::endl;

    std::vector<std::thread> threads;
    auto start_time = std::chrono::high_resolution_clock::now();

    // Launch worker threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker_thread);
    }

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    long long total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // Display benchmark results
    std::cout << "TPC-C Benchmark Complete" << std::endl;
    std::cout << "Total Transactions: " << NUM_TRANSACTIONS << std::endl;
    std::cout << "Successful Transactions: " << successful_transactions.load() << std::endl;
    std::cout << "Failed Transactions: " << failed_transactions.load() << std::endl;
    std::cout << "Total Time: " << total_time << " ms" << std::endl;
    std::cout << "Average Latency: " << (total_latency / successful_transactions.load()) << " µs" << std::endl;
}

int main() {
    // Initialize connection pool and other setup
    ConnectionPool::get_instance().initialize(NUM_THREADS);

    // Run the TPC-C benchmark
    run_benchmark();

    // Cleanup
    ConnectionPool::get_instance().shutdown();

    return 0;
}