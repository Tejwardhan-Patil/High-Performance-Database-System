#include <gtest/gtest.h>
#include "query_processor/parser/sql_parser.h"
#include "query_processor/optimizer/plan_generator.h"
#include "query_processor/executor/query_executor.h"
#include "query_processor/planner/logical_plan.h"
#include "query_processor/planner/physical_plan.h"

class QueryProcessorTest : public ::testing::Test {
protected:
    SQLParser sqlParser;
    PlanGenerator planGenerator;
    QueryExecutor queryExecutor;

    void SetUp() override {
        // Initialize components before each test
        sqlParser = SQLParser();          
        planGenerator = PlanGenerator();    
        queryExecutor = QueryExecutor();     

        queryExecutor.connectToTestDB("test_database");
    }

    void TearDown() override {
        queryExecutor.disconnectFromDB();
    }
};

// Test case to verify SQL parsing
TEST_F(QueryProcessorTest, TestSQLParsing) {
    std::string query = "SELECT * FROM users WHERE age > 30";
    auto ast = sqlParser.parse(query);

    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getType(), ASTType::SelectStatement);
    ASSERT_EQ(ast->getTableName(), "users");
    ASSERT_EQ(ast->getConditions().size(), 1);
}

// Test case to check basic query optimization
TEST_F(QueryProcessorTest, TestQueryOptimization) {
    std::string query = "SELECT * FROM users WHERE age > 30";
    auto ast = sqlParser.parse(query);

    LogicalPlan logicalPlan = planGenerator.generateLogicalPlan(ast);
    PhysicalPlan physicalPlan = planGenerator.generatePhysicalPlan(logicalPlan);

    ASSERT_GT(physicalPlan.getCost(), 0);
    ASSERT_TRUE(physicalPlan.isOptimized());
}

// Test case to check basic query execution
TEST_F(QueryProcessorTest, TestQueryExecution) {
    std::string query = "SELECT * FROM users WHERE age > 30";
    auto ast = sqlParser.parse(query);

    LogicalPlan logicalPlan = planGenerator.generateLogicalPlan(ast);
    PhysicalPlan physicalPlan = planGenerator.generatePhysicalPlan(logicalPlan);

    QueryResult result = queryExecutor.execute(physicalPlan);

    ASSERT_EQ(result.getRowCount(), 10);
    ASSERT_EQ(result.getColumnCount(), 5);
}

// Test case for join algorithm testing
TEST_F(QueryProcessorTest, TestJoinAlgorithmExecution) {
    std::string query = "SELECT * FROM users INNER JOIN orders ON users.id = orders.user_id";
    auto ast = sqlParser.parse(query);

    LogicalPlan logicalPlan = planGenerator.generateLogicalPlan(ast);
    PhysicalPlan physicalPlan = planGenerator.generatePhysicalPlan(logicalPlan);

    QueryResult result = queryExecutor.execute(physicalPlan);

    ASSERT_EQ(result.getRowCount(), 50);
    ASSERT_EQ(result.getColumnCount(), 7);
}

// Test case for query execution with filtering condition
TEST_F(QueryProcessorTest, TestFilteredQueryExecution) {
    std::string query = "SELECT * FROM users WHERE name LIKE 'Mike%'";
    auto ast = sqlParser.parse(query);

    LogicalPlan logicalPlan = planGenerator.generateLogicalPlan(ast);
    PhysicalPlan physicalPlan = planGenerator.generatePhysicalPlan(logicalPlan);

    QueryResult result = queryExecutor.execute(physicalPlan);

    ASSERT_EQ(result.getRowCount(), 5);
    ASSERT_EQ(result.getColumnCount(), 5);
}

// Test case to validate cost estimation
TEST_F(QueryProcessorTest, TestCostEstimation) {
    std::string query = "SELECT * FROM users WHERE age > 30";
    auto ast = sqlParser.parse(query);

    LogicalPlan logicalPlan = planGenerator.generateLogicalPlan(ast);
    PhysicalPlan physicalPlan = planGenerator.generatePhysicalPlan(logicalPlan);

    double cost = planGenerator.estimateCost(physicalPlan);

    ASSERT_GT(cost, 0);
}

// Test case to verify query optimization with indexes
TEST_F(QueryProcessorTest, TestQueryOptimizationWithIndex) {
    std::string query = "SELECT * FROM users WHERE id = 100";
    auto ast = sqlParser.parse(query);

    LogicalPlan logicalPlan = planGenerator.generateLogicalPlan(ast);
    PhysicalPlan physicalPlan = planGenerator.generatePhysicalPlan(logicalPlan);

    ASSERT_TRUE(physicalPlan.usesIndex());
    ASSERT_GT(physicalPlan.getCost(), 0);
}

// Test case for NoSQL query parsing
TEST_F(QueryProcessorTest, TestNoSQLParsing) {
    std::string query = "{find: 'users', filter: {age: {$gt: 30}}}";
    auto ast = sqlParser.parseNoSQL(query);

    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->getCollection(), "users");
    ASSERT_EQ(ast->getFilter().size(), 1);
}

// Test case for optimizing NoSQL queries
TEST_F(QueryProcessorTest, TestNoSQLQueryOptimization) {
    std::string query = "{find: 'users', filter: {age: {$gt: 30}}}";
    auto ast = sqlParser.parseNoSQL(query);

    LogicalPlan logicalPlan = planGenerator.generateLogicalPlan(ast);
    PhysicalPlan physicalPlan = planGenerator.generatePhysicalPlan(logicalPlan);

    ASSERT_GT(physicalPlan.getCost(), 0);
}

// Test case to verify NoSQL query execution
TEST_F(QueryProcessorTest, TestNoSQLQueryExecution) {
    std::string query = "{find: 'users', filter: {age: {$gt: 30}}}";
    auto ast = sqlParser.parseNoSQL(query);

    LogicalPlan logicalPlan = planGenerator.generateLogicalPlan(ast);
    PhysicalPlan physicalPlan = planGenerator.generatePhysicalPlan(logicalPlan);

    QueryResult result = queryExecutor.execute(physicalPlan);

    ASSERT_EQ(result.getRowCount(), 10);
}

// Test case for insert queries
TEST_F(QueryProcessorTest, TestInsertQueryExecution) {
    std::string query = "INSERT INTO users (id, name, age) VALUES (101, 'Max', 28)";
    auto ast = sqlParser.parse(query);

    LogicalPlan logicalPlan = planGenerator.generateLogicalPlan(ast);
    PhysicalPlan physicalPlan = planGenerator.generatePhysicalPlan(logicalPlan);

    QueryResult result = queryExecutor.execute(physicalPlan);

    ASSERT_TRUE(result.isSuccess());
    ASSERT_EQ(result.getAffectedRowCount(), 1);
}

// Test case for delete queries
TEST_F(QueryProcessorTest, TestDeleteQueryExecution) {
    std::string query = "DELETE FROM users WHERE id = 101";
    auto ast = sqlParser.parse(query);

    LogicalPlan logicalPlan = planGenerator.generateLogicalPlan(ast);
    PhysicalPlan physicalPlan = planGenerator.generatePhysicalPlan(logicalPlan);

    QueryResult result = queryExecutor.execute(physicalPlan);

    ASSERT_TRUE(result.isSuccess());
    ASSERT_EQ(result.getAffectedRowCount(), 1);
}

// Test case for update queries
TEST_F(QueryProcessorTest, TestUpdateQueryExecution) {
    std::string query = "UPDATE users SET name = 'Matt' WHERE id = 101";
    auto ast = sqlParser.parse(query);

    LogicalPlan logicalPlan = planGenerator.generateLogicalPlan(ast);
    PhysicalPlan physicalPlan = planGenerator.generatePhysicalPlan(logicalPlan);

    QueryResult result = queryExecutor.execute(physicalPlan);

    ASSERT_TRUE(result.isSuccess());
    ASSERT_EQ(result.getAffectedRowCount(), 1);
}

// Test case for aggregation queries
TEST_F(QueryProcessorTest, TestAggregationQueryExecution) {
    std::string query = "SELECT COUNT(*) FROM users";
    auto ast = sqlParser.parse(query);

    LogicalPlan logicalPlan = planGenerator.generateLogicalPlan(ast);
    PhysicalPlan physicalPlan = planGenerator.generatePhysicalPlan(logicalPlan);

    QueryResult result = queryExecutor.execute(physicalPlan);

    ASSERT_EQ(result.getSingleValue(), 100);
}