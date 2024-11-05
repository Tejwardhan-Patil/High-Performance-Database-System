#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <atomic>
#include "database.h"  

// Constants for stress test parameters
const int NUM_THREADS = 100;            
const int OPERATIONS_PER_THREAD = 1000; 
const int MAX_KEY = 100000;             

// Random number generator
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> key_dist(1, MAX_KEY);
std::uniform_int_distribution<> value_dist(1, 100000);

// Atomic variables to track operations
std::atomic<int> read_count(0);
std::atomic<int> write_count(0);
std::atomic<int> error_count(0);

// Function to simulate a read operation
void perform_read(Database& db) {
    int key = key_dist(gen);
    try {
        int value = db.read(key);
        read_count++;
    } catch (const std::exception& e) {
        error_count++;
    }
}

// Function to simulate a write operation
void perform_write(Database& db) {
    int key = key_dist(gen);
    int value = value_dist(gen);
    try {
        db.write(key, value);
        write_count++;
    } catch (const std::exception& e) {
        error_count++;
    }
}

// Stress test function for each thread
void stress_test(Database& db, int thread_id) {
    for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
        if (i % 2 == 0) {
            perform_read(db);
        } else {
            perform_write(db);
        }

        // Simulate random delay between operations
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

// Main function to execute the stress test
int main() {
    // Initialize the database
    Database db;
    db.connect("localhost", 5432); 

    // Start time measurement
    auto start_time = std::chrono::high_resolution_clock::now();

    // Launch threads to perform stress testing
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.push_back(std::thread(stress_test, std::ref(db), i));
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // End time measurement
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // Print test results
    std::cout << "Stress Test Completed\n";
    std::cout << "Total Threads: " << NUM_THREADS << "\n";
    std::cout << "Operations per Thread: " << OPERATIONS_PER_THREAD << "\n";
    std::cout << "Total Reads: " << read_count.load() << "\n";
    std::cout << "Total Writes: " << write_count.load() << "\n";
    std::cout << "Total Errors: " << error_count.load() << "\n";
    std::cout << "Test Duration: " << duration << " ms\n";

    return 0;
}

// Function to simulate network stress by running queries on multiple connections
void network_stress_test(Database& db) {
    int connections = 100;  // Number of simultaneous connections
    std::vector<std::thread> threads;

    for (int i = 0; i < connections; ++i) {
        threads.push_back(std::thread([&db]() {
            for (int j = 0; j < 1000; ++j) {
                try {
                    db.execute_query("SELECT * FROM test_table LIMIT 10");
                } catch (const std::exception& e) {
                    error_count++;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "Network Stress Test Completed\n";
}

// Function to simulate a long-running transaction stress test
void transaction_stress_test(Database& db) {
    std::vector<std::thread> threads;

    // Launch threads to perform long transactions
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.push_back(std::thread([&db]() {
            for (int j = 0; j < OPERATIONS_PER_THREAD / 10; ++j) {
                try {
                    db.begin_transaction();
                    for (int k = 0; k < 10; ++k) {
                        perform_write(db);
                    }
                    db.commit_transaction();
                } catch (const std::exception& e) {
                    db.rollback_transaction();
                    error_count++;
                }
            }
        }));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "Transaction Stress Test Completed\n";
}

// Function to simulate high-frequency query execution
void query_stress_test(Database& db) {
    std::vector<std::thread> threads;

    // Launch threads to execute queries continuously
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.push_back(std::thread([&db]() {
            for (int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                try {
                    db.execute_query("SELECT COUNT(*) FROM test_table");
                } catch (const std::exception& e) {
                    error_count++;
                }
            }
        }));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "Query Stress Test Completed\n";
}

// Extended test scenarios
void extended_stress_tests(Database& db) {
    std::cout << "Starting Extended Stress Tests...\n";

    // Network stress test
    network_stress_test(db);

    // Long-running transaction stress test
    transaction_stress_test(db);

    // High-frequency query execution test
    query_stress_test(db);

    std::cout << "All Extended Stress Tests Completed\n";
}