#include "query_executor.h"
#include "storage_engine.h"
#include "optimizer.h"
#include "join_algorithms.h"
#include "buffer_manager.h"
#include "cost_estimator.h"
#include "transaction_manager.h"

namespace QueryExecutor {

QueryExecutor::QueryExecutor(StorageEngine* storage_engine, Optimizer* optimizer, BufferManager* buffer_manager, TransactionManager* txn_manager) 
    : storage_engine_(storage_engine), optimizer_(optimizer), buffer_manager_(buffer_manager), txn_manager_(txn_manager) {}

ExecutionResult QueryExecutor::executeQuery(const QueryPlan& query_plan) {
    // Begin transaction
    txn_manager_->beginTransaction();
    
    ExecutionResult result;
    switch (query_plan.getType()) {
        case QueryType::SELECT:
            result = executeSelect(query_plan);
            break;
        case QueryType::INSERT:
            result = executeInsert(query_plan);
            break;
        case QueryType::UPDATE:
            result = executeUpdate(query_plan);
            break;
        case QueryType::DELETE:
            result = executeDelete(query_plan);
            break;
        default:
            throw std::invalid_argument("Unsupported query type");
    }
    
    // Commit transaction
    txn_manager_->commitTransaction();
    
    return result;
}

ExecutionResult QueryExecutor::executeSelect(const QueryPlan& query_plan) {
    const std::vector<LogicalOperator>& operators = query_plan.getLogicalOperators();
    ExecutionContext context;
    
    for (const auto& op : operators) {
        switch (op.getType()) {
            case LogicalOperatorType::SCAN:
                executeScan(op, context);
                break;
            case LogicalOperatorType::JOIN:
                executeJoin(op, context);
                break;
            case LogicalOperatorType::FILTER:
                executeFilter(op, context);
                break;
            case LogicalOperatorType::PROJECT:
                executeProject(op, context);
                break;
            default:
                throw std::invalid_argument("Unsupported logical operator");
        }
    }
    
    return context.result;
}

void QueryExecutor::executeScan(const LogicalOperator& op, ExecutionContext& context) {
    // Access storage engine to perform the scan operation
    auto table = storage_engine_->getTable(op.getTableName());
    auto predicate = op.getPredicate();
    
    context.result = table->scan(predicate);
}

void QueryExecutor::executeJoin(const LogicalOperator& op, ExecutionContext& context) {
    JoinAlgorithm* join_algorithm = nullptr;
    
    switch (op.getJoinType()) {
        case JoinType::HASH_JOIN:
            join_algorithm = new HashJoin();
            break;
        case JoinType::MERGE_JOIN:
            join_algorithm = new MergeJoin();
            break;
        default:
            throw std::invalid_argument("Unsupported join type");
    }
    
    auto left_table = context.result;
    auto right_table = storage_engine_->getTable(op.getRightTable());
    
    context.result = join_algorithm->execute(left_table, right_table, op.getJoinCondition());
    
    delete join_algorithm;
}

void QueryExecutor::executeFilter(const LogicalOperator& op, ExecutionContext& context) {
    // Apply filter on the current result set
    auto predicate = op.getPredicate();
    context.result = context.result.filter(predicate);
}

void QueryExecutor::executeProject(const LogicalOperator& op, ExecutionContext& context) {
    // Perform projection on the result set
    context.result = context.result.project(op.getProjectedColumns());
}

ExecutionResult QueryExecutor::executeInsert(const QueryPlan& query_plan) {
    // Insert operation
    auto table = storage_engine_->getTable(query_plan.getTableName());
    auto data = query_plan.getInsertData();
    
    table->insert(data);
    return ExecutionResult::success();
}

ExecutionResult QueryExecutor::executeUpdate(const QueryPlan& query_plan) {
    // Update operation
    auto table = storage_engine_->getTable(query_plan.getTableName());
    auto predicate = query_plan.getPredicate();
    auto update_data = query_plan.getUpdateData();
    
    table->update(predicate, update_data);
    return ExecutionResult::success();
}

ExecutionResult QueryExecutor::executeDelete(const QueryPlan& query_plan) {
    // Delete operation
    auto table = storage_engine_->getTable(query_plan.getTableName());
    auto predicate = query_plan.getPredicate();
    
    table->remove(predicate);
    return ExecutionResult::success();
}

// Helper method for transaction handling
void QueryExecutor::rollbackOnFailure() {
    txn_manager_->rollbackTransaction();
}

// Memory management and buffer handling
void QueryExecutor::manageMemory() {
    buffer_manager_->clearUnusedBuffers();
}

} // namespace QueryExecutor

// ==================== Header File: query_executor.h ====================

#ifndef QUERY_EXECUTOR_H
#define QUERY_EXECUTOR_H

#include "query_plan.h"
#include "execution_context.h"
#include "storage_engine.h"
#include "optimizer.h"
#include "buffer_manager.h"
#include "transaction_manager.h"

namespace QueryExecutor {

class QueryExecutor {
public:
    QueryExecutor(StorageEngine* storage_engine, Optimizer* optimizer, BufferManager* buffer_manager, TransactionManager* txn_manager);

    ExecutionResult executeQuery(const QueryPlan& query_plan);

private:
    StorageEngine* storage_engine_;
    Optimizer* optimizer_;
    BufferManager* buffer_manager_;
    TransactionManager* txn_manager_;
    
    ExecutionResult executeSelect(const QueryPlan& query_plan);
    ExecutionResult executeInsert(const QueryPlan& query_plan);
    ExecutionResult executeUpdate(const QueryPlan& query_plan);
    ExecutionResult executeDelete(const QueryPlan& query_plan);
    
    void executeScan(const LogicalOperator& op, ExecutionContext& context);
    void executeJoin(const LogicalOperator& op, ExecutionContext& context);
    void executeFilter(const LogicalOperator& op, ExecutionContext& context);
    void executeProject(const LogicalOperator& op, ExecutionContext& context);
    
    void rollbackOnFailure();
    void manageMemory();
};

} // namespace QueryExecutor

#endif // QUERY_EXECUTOR_H