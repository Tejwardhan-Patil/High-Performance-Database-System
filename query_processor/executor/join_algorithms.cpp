#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>

// Structure representing a row in the table
struct Row {
    int id;
    std::string value;

    Row(int id_, std::string value_) : id(id_), value(value_) {}
};

// Result of a join operation
struct JoinedRow {
    int id;
    std::string leftValue;
    std::string rightValue;

    JoinedRow(int id_, std::string leftValue_, std::string rightValue_)
        : id(id_), leftValue(leftValue_), rightValue(rightValue_) {}
};

// Helper function to print joined rows
void printJoinResult(const std::vector<JoinedRow>& result) {
    for (const auto& row : result) {
        std::cout << "ID: " << row.id 
                  << ", Left Value: " << row.leftValue 
                  << ", Right Value: " << row.rightValue << std::endl;
    }
}

// Hash Join Algorithm
std::vector<JoinedRow> hashJoin(const std::vector<Row>& leftTable, const std::vector<Row>& rightTable) {
    std::unordered_map<int, std::string> hashTable;
    std::vector<JoinedRow> joinResult;

    // Build phase: Build hash table on rightTable
    for (const auto& row : rightTable) {
        hashTable[row.id] = row.value;
    }

    // Probe phase: Iterate over leftTable and find matches in hash table
    for (const auto& leftRow : leftTable) {
        if (hashTable.find(leftRow.id) != hashTable.end()) {
            joinResult.emplace_back(leftRow.id, leftRow.value, hashTable[leftRow.id]);
        }
    }

    return joinResult;
}

// Merge Join Algorithm
std::vector<JoinedRow> mergeJoin(std::vector<Row>& leftTable, std::vector<Row>& rightTable) {
    std::vector<JoinedRow> joinResult;

    // Sort both tables on the join key (ID)
    std::sort(leftTable.begin(), leftTable.end(), [](const Row& a, const Row& b) { return a.id < b.id; });
    std::sort(rightTable.begin(), rightTable.end(), [](const Row& a, const Row& b) { return a.id < b.id; });

    // Perform merge join
    size_t leftIndex = 0, rightIndex = 0;

    while (leftIndex < leftTable.size() && rightIndex < rightTable.size()) {
        if (leftTable[leftIndex].id == rightTable[rightIndex].id) {
            joinResult.emplace_back(leftTable[leftIndex].id, leftTable[leftIndex].value, rightTable[rightIndex].value);
            ++leftIndex;
            ++rightIndex;
        } else if (leftTable[leftIndex].id < rightTable[rightIndex].id) {
            ++leftIndex;
        } else {
            ++rightIndex;
        }
    }

    return joinResult;
}

// Nested Loop Join Algorithm
std::vector<JoinedRow> nestedLoopJoin(const std::vector<Row>& leftTable, const std::vector<Row>& rightTable) {
    std::vector<JoinedRow> joinResult;

    // Iterate over every combination of leftTable and rightTable rows
    for (const auto& leftRow : leftTable) {
        for (const auto& rightRow : rightTable) {
            if (leftRow.id == rightRow.id) {
                joinResult.emplace_back(leftRow.id, leftRow.value, rightRow.value);
            }
        }
    }

    return joinResult;
}

// Helper function to generate test data for joins
void generateTestData(std::vector<Row>& leftTable, std::vector<Row>& rightTable) {
    // Generating sample data for the left table
    leftTable.emplace_back(1, "Left_One");
    leftTable.emplace_back(2, "Left_Two");
    leftTable.emplace_back(3, "Left_Three");

    // Generating sample data for the right table
    rightTable.emplace_back(2, "Right_Two");
    rightTable.emplace_back(3, "Right_Three");
    rightTable.emplace_back(4, "Right_Four");
}

int main() {
    std::vector<Row> leftTable;
    std::vector<Row> rightTable;

    // Generate sample data for tables
    generateTestData(leftTable, rightTable);

    std::cout << "Executing Hash Join:" << std::endl;
    auto hashJoinResult = hashJoin(leftTable, rightTable);
    printJoinResult(hashJoinResult);

    std::cout << "\nExecuting Merge Join:" << std::endl;
    auto mergeJoinResult = mergeJoin(leftTable, rightTable);
    printJoinResult(mergeJoinResult);

    std::cout << "\nExecuting Nested Loop Join:" << std::endl;
    auto nestedLoopJoinResult = nestedLoopJoin(leftTable, rightTable);
    printJoinResult(nestedLoopJoinResult);

    return 0;
}