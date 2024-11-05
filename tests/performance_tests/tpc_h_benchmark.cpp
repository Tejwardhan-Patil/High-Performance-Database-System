#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <thread>
#include "tpc_h_queries.h" 
#include "query_executor.h" 

using namespace std;
using namespace chrono;

class TPCHBenchmark {
public:
    TPCHBenchmark(int scale_factor, int num_threads)
        : scale_factor(scale_factor), num_threads(num_threads) {}

    void runBenchmark() {
        cout << "Starting TPC-H benchmark with scale factor: " << scale_factor << " and "
             << num_threads << " threads." << endl;
        
        auto start_time = high_resolution_clock::now();
        
        vector<thread> threads;
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back(&TPCHBenchmark::runQueries, this, i);
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end_time = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end_time - start_time).count();
        cout << "Benchmark completed in " << duration << " ms." << endl;
    }

private:
    int scale_factor;
    int num_threads;

    void runQueries(int thread_id) {
        cout << "Thread " << thread_id << " is running queries..." << endl;

        vector<string> queries = getTPCHQueries();
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dist(0, queries.size() - 1);

        for (int i = 0; i < scale_factor; ++i) {
            string query = queries[dist(gen)];
            executeQuery(query);
        }

        cout << "Thread " << thread_id << " finished running queries." << endl;
    }

    vector<string> getTPCHQueries() {
        return {
            "SELECT * FROM orders WHERE o_orderdate >= '1994-01-01' AND o_orderdate < '1995-01-01';",
            "SELECT * FROM customer WHERE c_acctbal > 5000;",
            "SELECT o_orderkey, SUM(l_extendedprice * (1 - l_discount)) FROM orders JOIN lineitem ON o_orderkey = l_orderkey GROUP BY o_orderkey;",
        };
    }

    void executeQuery(const string& query) {
        // Simulate query execution and log results
        auto start = high_resolution_clock::now();
        QueryExecutor executor;
        executor.execute(query);
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start).count();
        cout << "Query executed in " << duration << " ms." << endl;
    }
};

int main(int argc, char** argv) {
    if (argc != 3) {
        cout << "Usage: ./tpc_h_benchmark <scale_factor> <num_threads>" << endl;
        return 1;
    }

    int scale_factor = stoi(argv[1]);
    int num_threads = stoi(argv[2]);

    TPCHBenchmark benchmark(scale_factor, num_threads);
    benchmark.runBenchmark();

    return 0;
}