#include <iostream>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <limits>
#include <string>

// Cost Estimation Parameters
const double DISK_IO_COST = 5.0; // Cost for disk I/O operations
const double MEMORY_ACCESS_COST = 1.0; // Cost for accessing memory
const double CPU_COST = 0.5; // Cost for CPU processing
const double NETWORK_COST = 10.0; // Cost for network communication in distributed queries

// Plan Cost Struct
struct PlanCost {
    double disk_io_cost;
    double memory_access_cost;
    double cpu_cost;
    double network_cost;
    
    // Sum of all costs
    double total_cost() const {
        return disk_io_cost + memory_access_cost + cpu_cost + network_cost;
    }
};

// Logical Plan Node Type Enum
enum class NodeType {
    SCAN,
    JOIN,
    AGGREGATION,
    SORT,
    FILTER,
    INSERT,
    UPDATE,
    DELETE
};

// Logical Plan Node Structure
struct PlanNode {
    NodeType node_type;
    int rows;
    int width; // Size of a row in bytes
    double selectivity; // Fraction of rows passed through the node
    std::vector<PlanNode*> children;
};

// Cost Estimator Class
class CostEstimator {
public:
    CostEstimator() = default;

    PlanCost estimate_cost(const PlanNode* node) {
        switch (node->node_type) {
            case NodeType::SCAN:
                return estimate_scan_cost(node);
            case NodeType::JOIN:
                return estimate_join_cost(node);
            case NodeType::AGGREGATION:
                return estimate_aggregation_cost(node);
            case NodeType::SORT:
                return estimate_sort_cost(node);
            case NodeType::FILTER:
                return estimate_filter_cost(node);
            case NodeType::INSERT:
                return estimate_insert_cost(node);
            case NodeType::UPDATE:
                return estimate_update_cost(node);
            case NodeType::DELETE:
                return estimate_delete_cost(node);
            default:
                return PlanCost{0.0, 0.0, 0.0, 0.0};
        }
    }

private:
    // Estimate cost for a table scan
    PlanCost estimate_scan_cost(const PlanNode* node) {
        double disk_io = (node->rows * node->width) / 1024.0; // I/O cost in KB
        double memory_access = disk_io * 0.8; // 80% loaded into memory
        double cpu = node->rows * CPU_COST;
        return PlanCost{disk_io * DISK_IO_COST, memory_access * MEMORY_ACCESS_COST, cpu, 0.0};
    }

    // Estimate cost for a join operation
    PlanCost estimate_join_cost(const PlanNode* node) {
        PlanCost left_cost = estimate_cost(node->children[0]);
        PlanCost right_cost = estimate_cost(node->children[1]);
        double output_rows = node->rows;
        double memory_access = output_rows * node->width / 1024.0;
        double cpu = output_rows * CPU_COST * 2; // Joins are CPU intensive
        return PlanCost{
            left_cost.disk_io_cost + right_cost.disk_io_cost,
            left_cost.memory_access_cost + right_cost.memory_access_cost + memory_access * MEMORY_ACCESS_COST,
            left_cost.cpu_cost + right_cost.cpu_cost + cpu,
            0.0
        };
    }

    // Estimate cost for an aggregation operation
    PlanCost estimate_aggregation_cost(const PlanNode* node) {
        PlanCost input_cost = estimate_cost(node->children[0]);
        double cpu = node->rows * CPU_COST * 1.5; // Aggregations are moderately CPU-intensive
        return PlanCost{
            input_cost.disk_io_cost,
            input_cost.memory_access_cost,
            input_cost.cpu_cost + cpu,
            0.0
        };
    }

    // Estimate cost for a sort operation
    PlanCost estimate_sort_cost(const PlanNode* node) {
        PlanCost input_cost = estimate_cost(node->children[0]);
        double memory_access = node->rows * node->width / 512.0; // Twice the memory access for sorting
        double cpu = node->rows * CPU_COST * 2; // Sorting is CPU-intensive
        return PlanCost{
            input_cost.disk_io_cost,
            input_cost.memory_access_cost + memory_access * MEMORY_ACCESS_COST,
            input_cost.cpu_cost + cpu,
            0.0
        };
    }

    // Estimate cost for a filter operation
    PlanCost estimate_filter_cost(const PlanNode* node) {
        PlanCost input_cost = estimate_cost(node->children[0]);
        double output_rows = node->rows * node->selectivity;
        double cpu = output_rows * CPU_COST * 0.5; // Filtering is less CPU-intensive
        return PlanCost{
            input_cost.disk_io_cost,
            input_cost.memory_access_cost,
            input_cost.cpu_cost + cpu,
            0.0
        };
    }

    // Estimate cost for insert operation
    PlanCost estimate_insert_cost(const PlanNode* node) {
        double disk_io = (node->rows * node->width) / 1024.0;
        double cpu = node->rows * CPU_COST * 0.8; // Inserts are lightweight in terms of CPU
        return PlanCost{
            disk_io * DISK_IO_COST,
            0.0,
            cpu,
            0.0
        };
    }

    // Estimate cost for update operation
    PlanCost estimate_update_cost(const PlanNode* node) {
        double disk_io = (node->rows * node->width) / 1024.0;
        double cpu = node->rows * CPU_COST;
        return PlanCost{
            disk_io * DISK_IO_COST,
            0.0,
            cpu,
            0.0
        };
    }

    // Estimate cost for delete operation
    PlanCost estimate_delete_cost(const PlanNode* node) {
        double disk_io = (node->rows * node->width) / 1024.0;
        double cpu = node->rows * CPU_COST * 0.5; // Deletes are generally lightweight
        return PlanCost{
            disk_io * DISK_IO_COST,
            0.0,
            cpu,
            0.0
        };
    }
};

// Usage
int main() {
    PlanNode scan_node{NodeType::SCAN, 100000, 128, 1.0, {}};
    PlanNode join_node{NodeType::JOIN, 50000, 256, 1.0, {&scan_node, &scan_node}};
    PlanNode agg_node{NodeType::AGGREGATION, 5000, 256, 1.0, {&join_node}};

    CostEstimator estimator;
    PlanCost agg_cost = estimator.estimate_cost(&agg_node);

    std::cout << "Total cost of aggregation plan: " << agg_cost.total_cost() << std::endl;
    std::cout << "Disk I/O cost: " << agg_cost.disk_io_cost << std::endl;
    std::cout << "Memory access cost: " << agg_cost.memory_access_cost << std::endl;
    std::cout << "CPU cost: " << agg_cost.cpu_cost << std::endl;
    std::cout << "Network cost: " << agg_cost.network_cost << std::endl;

    return 0;
}