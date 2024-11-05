#include <iostream>
#include <cassert>
#include "../transactions/transactions.h"
#include "../query_processor/query_executor.h"
#include "../storage/disk_storage/data_file_manager.h"
#include "../storage/buffer_manager.h"
#include "../indexing/b_tree/b_tree_index.h"
#include "../logging/write_ahead_log.h"

// Integration Test for Transactions and Query Execution
void testTransactionAndQueryExecution() {
    // Initialize storage and buffer manager
    DataFileManager dataFileManager;
    BufferManager bufferManager;

    // Initialize B-tree index
    BTreeIndex bTreeIndex;

    // Initialize Write Ahead Log (WAL) for crash recovery
    WriteAheadLog wal;

    // Create a transaction and start a transaction
    Transactions transactionManager;
    Transaction txn = transactionManager.beginTransaction();

    // Execute a query (inserting data into storage)
    QueryExecutor queryExecutor;
    std::string query = "INSERT INTO table_name VALUES (1, 'Test')";
    bool querySuccess = queryExecutor.execute(query, txn);

    // Assert that the query execution was successful
    assert(querySuccess == true);

    // Commit the transaction
    bool commitSuccess = transactionManager.commit(txn);
    assert(commitSuccess == true);

    // Check that data has been written to disk and indexed
    bool dataExists = dataFileManager.verifyDataExists("table_name", 1);
    assert(dataExists == true);

    bool indexUpdated = bTreeIndex.verifyIndexUpdated("table_name", 1);
    assert(indexUpdated == true);

    // Ensure WAL is cleared after the commit
    bool walCleared = wal.verifyLogCleared(txn);
    assert(walCleared == true);

    std::cout << "Test Transaction and Query Execution passed.\n";
}

// Integration Test for Crash Recovery with WAL
void testCrashRecoveryWithWAL() {
    // Initialize storage, buffer manager, and WAL
    DataFileManager dataFileManager;
    BufferManager bufferManager;
    WriteAheadLog wal;

    // Simulate a transaction before the crash
    Transactions transactionManager;
    Transaction txn = transactionManager.beginTransaction();

    QueryExecutor queryExecutor;
    std::string query = "INSERT INTO table_name VALUES (2, 'Crash Test')";
    bool querySuccess = queryExecutor.execute(query, txn);
    assert(querySuccess == true);

    // Simulate a crash (no commit)
    bool crashOccurred = transactionManager.simulateCrash(txn);
    assert(crashOccurred == true);

    // Recover from crash using WAL
    bool recoverySuccess = wal.recoverFromLog();
    assert(recoverySuccess == true);

    // Verify data integrity after recovery
    bool dataRecovered = dataFileManager.verifyDataExists("table_name", 2);
    assert(dataRecovered == false);  // Should be false since it wasn't committed

    std::cout << "Test Crash Recovery with WAL passed.\n";
}

// Integration Test for B-Tree Index Consistency
void testBTreeIndexConsistency() {
    // Initialize storage and B-tree index
    DataFileManager dataFileManager;
    BTreeIndex bTreeIndex;

    // Insert data and update the index
    Transactions transactionManager;
    Transaction txn = transactionManager.beginTransaction();

    QueryExecutor queryExecutor;
    std::string query1 = "INSERT INTO table_name VALUES (3, 'Index Test 1')";
    std::string query2 = "INSERT INTO table_name VALUES (4, 'Index Test 2')";
    queryExecutor.execute(query1, txn);
    queryExecutor.execute(query2, txn);

    transactionManager.commit(txn);

    // Verify that the index has been updated
    bool indexUpdated1 = bTreeIndex.verifyIndexUpdated("table_name", 3);
    bool indexUpdated2 = bTreeIndex.verifyIndexUpdated("table_name", 4);
    assert(indexUpdated1 == true);
    assert(indexUpdated2 == true);

    // Remove data and check the index consistency
    Transaction txn2 = transactionManager.beginTransaction();
    std::string deleteQuery = "DELETE FROM table_name WHERE id = 3";
    queryExecutor.execute(deleteQuery, txn2);
    transactionManager.commit(txn2);

    // Verify that the index has been updated after deletion
    bool indexDeleted = bTreeIndex.verifyIndexUpdated("table_name", 3);
    assert(indexDeleted == false);

    std::cout << "Test B-Tree Index Consistency passed.\n";
}

// Integration Test for Multiple Transaction Isolation Levels
void testTransactionIsolationLevels() {
    // Initialize components
    DataFileManager dataFileManager;
    Transactions transactionManager;
    QueryExecutor queryExecutor;

    // Begin Transaction 1 (Serializable Isolation)
    Transaction txn1 = transactionManager.beginTransaction(IsolationLevel::SERIALIZABLE);
    std::string query1 = "INSERT INTO table_name VALUES (5, 'Serializable Test')";
    queryExecutor.execute(query1, txn1);

    // Begin Transaction 2 (Read Committed Isolation)
    Transaction txn2 = transactionManager.beginTransaction(IsolationLevel::READ_COMMITTED);
    std::string query2 = "SELECT * FROM table_name WHERE id = 5";
    bool txn2Result = queryExecutor.execute(query2, txn2);
    assert(txn2Result == false); // Should be false due to isolation

    // Commit Transaction 1
    transactionManager.commit(txn1);

    // Retry Transaction 2 (should now succeed)
    txn2Result = queryExecutor.execute(query2, txn2);
    assert(txn2Result == true);

    // Commit Transaction 2
    transactionManager.commit(txn2);

    std::cout << "Test Transaction Isolation Levels passed.\n";
}

// Main function to run all integration tests
int main() {
    testTransactionAndQueryExecution();
    testCrashRecoveryWithWAL();
    testBTreeIndexConsistency();
    testTransactionIsolationLevels();

    std::cout << "All integration tests passed.\n";
    return 0;
}