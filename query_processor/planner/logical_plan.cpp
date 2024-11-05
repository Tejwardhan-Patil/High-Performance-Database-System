#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

// Enum representing different types of operations in the logical plan
enum class LogicalOperationType {
    SCAN,
    PROJECT,
    FILTER,
    JOIN,
    AGGREGATE,
    SORT
};

// Base class for all logical operations
class LogicalOperation {
public:
    explicit LogicalOperation(LogicalOperationType type) : opType(type) {}
    virtual ~LogicalOperation() = default;

    LogicalOperationType getOperationType() const {
        return opType;
    }

    virtual void execute() = 0;  // Virtual function to execute the operation

protected:
    LogicalOperationType opType;
};

// Logical Scan Operation
class LogicalScan : public LogicalOperation {
public:
    LogicalScan(const std::string& tableName)
        : LogicalOperation(LogicalOperationType::SCAN), tableName(tableName) {}

    void execute() override {
        std::cout << "Scanning table: " << tableName << std::endl;
    }

private:
    std::string tableName;
};

// Logical Project Operation
class LogicalProject : public LogicalOperation {
public:
    LogicalProject(const std::vector<std::string>& columns)
        : LogicalOperation(LogicalOperationType::PROJECT), columns(columns) {}

    void execute() override {
        std::cout << "Projecting columns: ";
        for (const auto& col : columns) {
            std::cout << col << " ";
        }
        std::cout << std::endl;
    }

private:
    std::vector<std::string> columns;
};

// Logical Filter Operation
class LogicalFilter : public LogicalOperation {
public:
    LogicalFilter(const std::string& condition)
        : LogicalOperation(LogicalOperationType::FILTER), condition(condition) {}

    void execute() override {
        std::cout << "Applying filter: " << condition << std::endl;
    }

private:
    std::string condition;
};

// Logical Join Operation
class LogicalJoin : public LogicalOperation {
public:
    LogicalJoin(const std::string& joinType, const std::string& leftTable, const std::string& rightTable, const std::string& condition)
        : LogicalOperation(LogicalOperationType::JOIN), joinType(joinType), leftTable(leftTable), rightTable(rightTable), condition(condition) {}

    void execute() override {
        std::cout << "Performing " << joinType << " join between " << leftTable << " and " << rightTable
                  << " on condition: " << condition << std::endl;
    }

private:
    std::string joinType;
    std::string leftTable;
    std::string rightTable;
    std::string condition;
};

// Logical Aggregate Operation
class LogicalAggregate : public LogicalOperation {
public:
    LogicalAggregate(const std::vector<std::string>& groupByColumns, const std::string& aggregateFunction, const std::string& targetColumn)
        : LogicalOperation(LogicalOperationType::AGGREGATE), groupByColumns(groupByColumns), aggregateFunction(aggregateFunction), targetColumn(targetColumn) {}

    void execute() override {
        std::cout << "Performing aggregation (" << aggregateFunction << ") on column: " << targetColumn
                  << " with group by: ";
        for (const auto& col : groupByColumns) {
            std::cout << col << " ";
        }
        std::cout << std::endl;
    }

private:
    std::vector<std::string> groupByColumns;
    std::string aggregateFunction;
    std::string targetColumn;
};

// Logical Sort Operation
class LogicalSort : public LogicalOperation {
public:
    LogicalSort(const std::vector<std::string>& orderByColumns, bool ascending = true)
        : LogicalOperation(LogicalOperationType::SORT), orderByColumns(orderByColumns), ascending(ascending) {}

    void execute() override {
        std::cout << "Sorting by columns: ";
        for (const auto& col : orderByColumns) {
            std::cout << col << " ";
        }
        std::cout << (ascending ? "ASC" : "DESC") << std::endl;
    }

private:
    std::vector<std::string> orderByColumns;
    bool ascending;
};

// Logical Plan Node, which can represent any logical operation in the query plan
class LogicalPlanNode {
public:
    explicit LogicalPlanNode(std::shared_ptr<LogicalOperation> operation) : operation(std::move(operation)) {}

    void addChild(std::shared_ptr<LogicalPlanNode> child) {
        children.push_back(std::move(child));
    }

    void execute() {
        operation->execute();
        for (const auto& child : children) {
            child->execute();
        }
    }

private:
    std::shared_ptr<LogicalOperation> operation;
    std::vector<std::shared_ptr<LogicalPlanNode>> children;
};

// Logical Plan class which holds the root node
class LogicalPlan {
public:
    explicit LogicalPlan(std::shared_ptr<LogicalPlanNode> root) : rootNode(std::move(root)) {}

    void execute() {
        if (rootNode) {
            rootNode->execute();
        }
    }

private:
    std::shared_ptr<LogicalPlanNode> rootNode;
};

// Logical plan construction and execution
int main() {
    // Create logical operations
    auto scanOp = std::make_shared<LogicalScan>("Employees");
    auto filterOp = std::make_shared<LogicalFilter>("salary > 50000");
    auto projectOp = std::make_shared<LogicalProject>(std::vector<std::string>{"name", "salary"});
    auto sortOp = std::make_shared<LogicalSort>(std::vector<std::string>{"salary"}, true);

    // Create nodes for logical plan
    auto scanNode = std::make_shared<LogicalPlanNode>(scanOp);
    auto filterNode = std::make_shared<LogicalPlanNode>(filterOp);
    auto projectNode = std::make_shared<LogicalPlanNode>(projectOp);
    auto sortNode = std::make_shared<LogicalPlanNode>(sortOp);

    // Build logical plan tree
    filterNode->addChild(scanNode);
    projectNode->addChild(filterNode);
    sortNode->addChild(projectNode);

    // Create logical plan with the root node
    LogicalPlan logicalPlan(sortNode);

    // Execute the plan
    logicalPlan.execute();

    return 0;
}