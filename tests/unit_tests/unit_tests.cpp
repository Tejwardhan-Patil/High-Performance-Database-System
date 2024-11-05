#include <gtest/gtest.h>
#include "storage/disk_storage/data_file_manager.h"
#include "storage/disk_storage/log_file_manager.h"
#include "indexing/b_tree/b_tree_index.h"
#include "query_processor/parser/sql_parser.h"
#include "transactions/concurrency_control/two_phase_locking.h"

// Test for Data File Manager in Storage Engine
TEST(StorageEngineTests, DataFileCreationTest) {
    DataFileManager dfm;
    bool result = dfm.createFile("test_data_file");
    ASSERT_TRUE(result);
}

TEST(StorageEngineTests, DataFileDeletionTest) {
    DataFileManager dfm;
    dfm.createFile("test_data_file");
    bool result = dfm.deleteFile("test_data_file");
    ASSERT_TRUE(result);
}

// Test for Log File Manager in Storage Engine
TEST(StorageEngineTests, LogFileWriteTest) {
    LogFileManager lfm;
    bool result = lfm.writeLog("Transaction 123", "Commit");
    ASSERT_TRUE(result);
}

TEST(StorageEngineTests, LogFileReadTest) {
    LogFileManager lfm;
    lfm.writeLog("Transaction 123", "Commit");
    std::string log_entry = lfm.readLog("Transaction 123");
    ASSERT_EQ(log_entry, "Commit");
}

// Test for B-Tree Index in Indexing
TEST(IndexingTests, BTreeInsertTest) {
    BTreeIndex btree;
    bool result = btree.insert(50, "value1");
    ASSERT_TRUE(result);
}

TEST(IndexingTests, BTreeSearchTest) {
    BTreeIndex btree;
    btree.insert(50, "value1");
    std::string result = btree.search(50);
    ASSERT_EQ(result, "value1");
}

TEST(IndexingTests, BTreeDeleteTest) {
    BTreeIndex btree;
    btree.insert(50, "value1");
    bool result = btree.remove(50);
    ASSERT_TRUE(result);
}

// Test for SQL Parser in Query Processor
TEST(QueryProcessorTests, SQLParsingTest) {
    SQLParser parser;
    std::string query = "SELECT * FROM users WHERE id = 1";
    auto ast = parser.parse(query);
    ASSERT_NE(ast, nullptr);
}

TEST(QueryProcessorTests, SQLInvalidParsingTest) {
    SQLParser parser;
    std::string invalid_query = "SELEC * FROM";
    auto ast = parser.parse(invalid_query);
    ASSERT_EQ(ast, nullptr);
}

// Test for Two-Phase Locking in Transactions
TEST(TransactionTests, LockAcquireTest) {
    TwoPhaseLocking twoPL;
    bool result = twoPL.acquireLock(123, "WRITE");
    ASSERT_TRUE(result);
}

TEST(TransactionTests, LockReleaseTest) {
    TwoPhaseLocking twoPL;
    twoPL.acquireLock(123, "WRITE");
    bool result = twoPL.releaseLock(123);
    ASSERT_TRUE(result);
}

// Test for Buffer Manager in Storage Engine
TEST(StorageEngineTests, BufferManagerCacheTest) {
    BufferManager buffer;
    buffer.loadPage(10);
    bool is_cached = buffer.isPageCached(10);
    ASSERT_TRUE(is_cached);
}

TEST(StorageEngineTests, BufferManagerEvictionTest) {
    BufferManager buffer;
    buffer.loadPage(10);
    buffer.evictPage(10);
    bool is_cached = buffer.isPageCached(10);
    ASSERT_FALSE(is_cached);
}

// Test for Query Executor in Query Processor
TEST(QueryProcessorTests, QueryExecutionTest) {
    QueryExecutor executor;
    std::string query = "SELECT name FROM users WHERE id = 1";
    auto result = executor.execute(query);
    ASSERT_NE(result, nullptr);
}

TEST(QueryProcessorTests, QueryExecutionFailureTest) {
    QueryExecutor executor;
    std::string invalid_query = "INVALID QUERY";
    auto result = executor.execute(invalid_query);
    ASSERT_EQ(result, nullptr);
}

// Test for Two-Phase Locking concurrency control
TEST(TransactionTests, ConcurrencyControlTest) {
    TwoPhaseLocking twoPL;
    bool lock1 = twoPL.acquireLock(123, "WRITE");
    bool lock2 = twoPL.acquireLock(124, "READ");
    ASSERT_TRUE(lock1);
    ASSERT_TRUE(lock2);
}

TEST(TransactionTests, DeadlockDetectionTest) {
    TwoPhaseLocking twoPL;
    twoPL.acquireLock(123, "WRITE");
    twoPL.acquireLock(124, "READ");
    bool deadlock_detected = twoPL.detectDeadlock();
    ASSERT_FALSE(deadlock_detected);
}

// Test for Write-Ahead Log in Transactions
TEST(TransactionTests, WALWriteTest) {
    WriteAheadLog wal;
    bool result = wal.write("Transaction 1", "START");
    ASSERT_TRUE(result);
}

TEST(TransactionTests, WALReadTest) {
    WriteAheadLog wal;
    wal.write("Transaction 1", "START");
    std::string entry = wal.read("Transaction 1");
    ASSERT_EQ(entry, "START");
}

// Test for Cost Estimator in Query Optimization
TEST(QueryOptimizationTests, CostEstimationTest) {
    CostEstimator estimator;
    double cost = estimator.estimateCost("SELECT * FROM users WHERE id = 1");
    ASSERT_GT(cost, 0.0);
}

TEST(QueryOptimizationTests, PlanGenerationTest) {
    PlanGenerator planGen;
    std::string query = "SELECT * FROM users";
    auto plan = planGen.generatePlan(query);
    ASSERT_NE(plan, nullptr);
}

// Test for Logical Plan in Query Planner
TEST(QueryPlannerTests, LogicalPlanTest) {
    LogicalPlan plan;
    plan.addOperation("Scan", "users");
    ASSERT_EQ(plan.getOperations().size(), 1);
}

TEST(QueryPlannerTests, PhysicalPlanTest) {
    PhysicalPlan plan;
    plan.addStep("HashJoin", "users", "orders");
    ASSERT_EQ(plan.getSteps().size(), 1);
}

// Test for Cache Mechanisms
TEST(CachingTests, LRUCacheTest) {
    LRUCache cache;
    cache.put(1, "data1");
    ASSERT_EQ(cache.get(1), "data1");
}

TEST(CachingTests, LFUCacheEvictionTest) {
    LFUCache cache;
    cache.put(1, "data1");
    cache.put(2, "data2");
    cache.put(3, "data3");
    cache.evict();
    ASSERT_EQ(cache.get(1), "data1");  // Should be still present
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}