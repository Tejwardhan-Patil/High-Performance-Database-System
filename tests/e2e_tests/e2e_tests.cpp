#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "../core/storage/disk_storage/data_file_manager.h"
#include "../core/query_processor/query_executor.h"
#include "../core/transactions/transactions.h"
#include "../core/indexing/b_tree/b_tree_index.h"
#include "../core/networking/rpc/rpc_client.h"

// Utility function to print test results
void printTestResult(const std::string &testName, bool result) {
    if (result) {
        std::cout << "[PASS] " << testName << std::endl;
    } else {
        std::cout << "[FAIL] " << testName << std::endl;
    }
}

// Test case 1: Check data insertion and retrieval
bool testInsertAndRetrieve() {
    // Setup
    DataFileManager dataManager;
    QueryExecutor queryExecutor;
    BTreeIndex index;
    
    // Insert data
    std::string queryInsert = "INSERT INTO users (id, name, age) VALUES (1, 'Person1', 30)";
    bool insertResult = queryExecutor.execute(queryInsert);
    
    // Verify insertion
    std::string querySelect = "SELECT name, age FROM users WHERE id = 1";
    std::vector<std::vector<std::string>> result = queryExecutor.executeQuery(querySelect);
    
    // Assert the data is correct
    return result.size() == 1 && result[0][0] == "Person1" && result[0][1] == "30";
}

// Test case 2: Check transaction commit and rollback
bool testTransactionCommitAndRollback() {
    // Setup
    Transactions transactionManager;
    
    // Begin a transaction
    transactionManager.begin();
    
    // Insert data
    std::string queryInsert = "INSERT INTO orders (order_id, user_id, amount) VALUES (1, 1, 100)";
    bool insertResult = transactionManager.execute(queryInsert);
    
    // Commit transaction
    transactionManager.commit();
    
    // Verify data is inserted after commit
    std::string querySelect = "SELECT order_id, amount FROM orders WHERE order_id = 1";
    std::vector<std::vector<std::string>> resultAfterCommit = transactionManager.executeQuery(querySelect);
    
    if (resultAfterCommit.size() != 1 || resultAfterCommit[0][0] != "1" || resultAfterCommit[0][1] != "100") {
        return false;
    }
    
    // Begin another transaction and roll it back
    transactionManager.begin();
    std::string queryInsertRollback = "INSERT INTO orders (order_id, user_id, amount) VALUES (2, 1, 200)";
    transactionManager.execute(queryInsertRollback);
    transactionManager.rollback();
    
    // Verify data is not inserted after rollback
    std::string querySelectRollback = "SELECT order_id FROM orders WHERE order_id = 2";
    std::vector<std::vector<std::string>> resultAfterRollback = transactionManager.executeQuery(querySelectRollback);
    
    return resultAfterRollback.empty();
}

// Test case 3: Check index creation and search
bool testIndexCreationAndSearch() {
    // Setup
    BTreeIndex index;
    
    // Create index
    index.createIndex("users", "name");
    
    // Insert data
    std::string queryInsert = "INSERT INTO users (id, name, age) VALUES (2, 'Person2', 25)";
    QueryExecutor queryExecutor;
    queryExecutor.execute(queryInsert);
    
    // Search using index
    std::string querySelect = "SELECT id FROM users WHERE name = 'Person2'";
    std::vector<std::vector<std::string>> result = queryExecutor.executeQuery(querySelect);
    
    // Verify the result
    return result.size() == 1 && result[0][0] == "2";
}

// Test case 4: Check network RPC communication
bool testRpcCommunication() {
    // Setup
    RpcClient rpcClient;
    
    // Make an RPC call to retrieve data from a remote node
    std::string response = rpcClient.sendRequest("GET /data?userId=1");
    
    // Verify the response
    return response == "{\"id\": 1, \"name\": \"Person1\", \"age\": 30}";
}

// Test case 5: Check query optimization
bool testQueryOptimization() {
    // Setup
    QueryExecutor queryExecutor;
    
    // Execute a complex query
    std::string complexQuery = "SELECT users.name, orders.amount FROM users JOIN orders ON users.id = orders.user_id";
    queryExecutor.enableOptimization();
    auto start = std::chrono::high_resolution_clock::now();
    queryExecutor.execute(complexQuery);
    auto end = std::chrono::high_resolution_clock::now();
    
    // Verify the execution time is reasonable
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    return duration < 100; // Assume optimized query should take less than 100 ms
}

// Test case 6: Check data recovery after failure
bool testRecoveryAfterFailure() {
    // Setup
    DataFileManager dataManager;
    Transactions transactionManager;
    
    // Simulate a crash during a transaction
    transactionManager.begin();
    std::string queryInsert = "INSERT INTO transactions (txn_id, user_id, status) VALUES (1, 1, 'pending')";
    transactionManager.execute(queryInsert);
    transactionManager.simulateCrash();
    
    // Recover the system
    transactionManager.recover();
    
    // Verify data is consistent after recovery
    std::string querySelect = "SELECT status FROM transactions WHERE txn_id = 1";
    std::vector<std::vector<std::string>> result = transactionManager.executeQuery(querySelect);
    
    return result.empty(); // The transaction should not have been committed
}

// Main function to run all tests
int main() {
    bool allTestsPassed = true;
    
    // Run individual test cases
    allTestsPassed &= testInsertAndRetrieve();
    printTestResult("testInsertAndRetrieve", allTestsPassed);
    
    allTestsPassed &= testTransactionCommitAndRollback();
    printTestResult("testTransactionCommitAndRollback", allTestsPassed);
    
    allTestsPassed &= testIndexCreationAndSearch();
    printTestResult("testIndexCreationAndSearch", allTestsPassed);
    
    allTestsPassed &= testRpcCommunication();
    printTestResult("testRpcCommunication", allTestsPassed);
    
    allTestsPassed &= testQueryOptimization();
    printTestResult("testQueryOptimization", allTestsPassed);
    
    allTestsPassed &= testRecoveryAfterFailure();
    printTestResult("testRecoveryAfterFailure", allTestsPassed);
    
    // Final result
    if (allTestsPassed) {
        std::cout << "All E2E tests passed successfully." << std::endl;
    } else {
        std::cout << "Some E2E tests failed." << std::endl;
    }
    
    return allTestsPassed ? 0 : 1;
}