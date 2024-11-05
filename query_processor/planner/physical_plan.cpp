#include "physical_plan.h"
#include "storage_engine.h"
#include "index_manager.h"
#include "join_algorithms.h"
#include "cost_estimator.h"

PhysicalPlan::PhysicalPlan() {}

PhysicalPlanNode* PhysicalPlan::createSequentialScanPlan(Table* table) {
    PhysicalPlanNode* scanNode = new PhysicalPlanNode();
    scanNode->type = PlanNodeType::SequentialScan;
    scanNode->table = table;
    scanNode->estimatedCost = CostEstimator::estimateSequentialScanCost(table);
    return scanNode;
}

PhysicalPlanNode* PhysicalPlan::createIndexScanPlan(Table* table, Index* index) {
    PhysicalPlanNode* scanNode = new PhysicalPlanNode();
    scanNode->type = PlanNodeType::IndexScan;
    scanNode->table = table;
    scanNode->index = index;
    scanNode->estimatedCost = CostEstimator::estimateIndexScanCost(table, index);
    return scanNode;
}

PhysicalPlanNode* PhysicalPlan::createJoinPlan(PhysicalPlanNode* leftPlan, PhysicalPlanNode* rightPlan, JoinType joinType) {
    PhysicalPlanNode* joinNode = new PhysicalPlanNode();
    joinNode->type = PlanNodeType::Join;
    joinNode->leftChild = leftPlan;
    joinNode->rightChild = rightPlan;
    joinNode->joinType = joinType;

    switch (joinType) {
        case JoinType::HashJoin:
            joinNode->joinAlgorithm = new HashJoin();
            joinNode->estimatedCost = CostEstimator::estimateHashJoinCost(leftPlan, rightPlan);
            break;
        case JoinType::MergeJoin:
            joinNode->joinAlgorithm = new MergeJoin();
            joinNode->estimatedCost = CostEstimator::estimateMergeJoinCost(leftPlan, rightPlan);
            break;
        case JoinType::NestedLoopJoin:
            joinNode->joinAlgorithm = new NestedLoopJoin();
            joinNode->estimatedCost = CostEstimator::estimateNestedLoopJoinCost(leftPlan, rightPlan);
            break;
        default:
            throw std::invalid_argument("Invalid join type");
    }

    return joinNode;
}

PhysicalPlanNode* PhysicalPlan::createFilterPlan(PhysicalPlanNode* inputPlan, const FilterCondition& condition) {
    PhysicalPlanNode* filterNode = new PhysicalPlanNode();
    filterNode->type = PlanNodeType::Filter;
    filterNode->input = inputPlan;
    filterNode->filterCondition = condition;
    filterNode->estimatedCost = CostEstimator::estimateFilterCost(inputPlan, condition);
    return filterNode;
}

PhysicalPlanNode* PhysicalPlan::createProjectionPlan(PhysicalPlanNode* inputPlan, const std::vector<Column>& columns) {
    PhysicalPlanNode* projectionNode = new PhysicalPlanNode();
    projectionNode->type = PlanNodeType::Projection;
    projectionNode->input = inputPlan;
    projectionNode->projectionColumns = columns;
    projectionNode->estimatedCost = CostEstimator::estimateProjectionCost(inputPlan, columns);
    return projectionNode;
}

PhysicalPlanNode* PhysicalPlan::createSortPlan(PhysicalPlanNode* inputPlan, const std::vector<SortColumn>& sortColumns) {
    PhysicalPlanNode* sortNode = new PhysicalPlanNode();
    sortNode->type = PlanNodeType::Sort;
    sortNode->input = inputPlan;
    sortNode->sortColumns = sortColumns;
    sortNode->estimatedCost = CostEstimator::estimateSortCost(inputPlan, sortColumns);
    return sortNode;
}

PhysicalPlanNode* PhysicalPlan::createLimitPlan(PhysicalPlanNode* inputPlan, int limit) {
    PhysicalPlanNode* limitNode = new PhysicalPlanNode();
    limitNode->type = PlanNodeType::Limit;
    limitNode->input = inputPlan;
    limitNode->limit = limit;
    limitNode->estimatedCost = CostEstimator::estimateLimitCost(inputPlan, limit);
    return limitNode;
}

PhysicalPlanNode* PhysicalPlan::createAggregatePlan(PhysicalPlanNode* inputPlan, const std::vector<AggregateFunction>& aggregateFunctions) {
    PhysicalPlanNode* aggregateNode = new PhysicalPlanNode();
    aggregateNode->type = PlanNodeType::Aggregate;
    aggregateNode->input = inputPlan;
    aggregateNode->aggregateFunctions = aggregateFunctions;
    aggregateNode->estimatedCost = CostEstimator::estimateAggregateCost(inputPlan, aggregateFunctions);
    return aggregateNode;
}

void PhysicalPlan::optimizePlan(PhysicalPlanNode* root) {
    // Basic optimization: push down filters
    if (root->type == PlanNodeType::Filter) {
        pushDownFilter(root);
    }

    // Recursive optimization of children
    if (root->leftChild) optimizePlan(root->leftChild);
    if (root->rightChild) optimizePlan(root->rightChild);
    if (root->input) optimizePlan(root->input);
}

void PhysicalPlan::pushDownFilter(PhysicalPlanNode* filterNode) {
    PhysicalPlanNode* childNode = filterNode->input;

    if (childNode->type == PlanNodeType::Join) {
        // Push down filter on left or right child based on filter condition
        if (canPushFilter(filterNode->filterCondition, childNode->leftChild)) {
            PhysicalPlanNode* newFilterNode = createFilterPlan(childNode->leftChild, filterNode->filterCondition);
            childNode->leftChild = newFilterNode;
        } else if (canPushFilter(filterNode->filterCondition, childNode->rightChild)) {
            PhysicalPlanNode* newFilterNode = createFilterPlan(childNode->rightChild, filterNode->filterCondition);
            childNode->rightChild = newFilterNode;
        }
    }
}

bool PhysicalPlan::canPushFilter(const FilterCondition& condition, PhysicalPlanNode* node) {
    // Determine if a filter can be pushed down based on column and node properties
    return condition.appliesTo(node);
}

PhysicalPlanNode* PhysicalPlan::createUnionPlan(PhysicalPlanNode* leftPlan, PhysicalPlanNode* rightPlan) {
    PhysicalPlanNode* unionNode = new PhysicalPlanNode();
    unionNode->type = PlanNodeType::Union;
    unionNode->leftChild = leftPlan;
    unionNode->rightChild = rightPlan;
    unionNode->estimatedCost = CostEstimator::estimateUnionCost(leftPlan, rightPlan);
    return unionNode;
}

PhysicalPlanNode* PhysicalPlan::createIntersectionPlan(PhysicalPlanNode* leftPlan, PhysicalPlanNode* rightPlan) {
    PhysicalPlanNode* intersectionNode = new PhysicalPlanNode();
    intersectionNode->type = PlanNodeType::Intersection;
    intersectionNode->leftChild = leftPlan;
    intersectionNode->rightChild = rightPlan;
    intersectionNode->estimatedCost = CostEstimator::estimateIntersectionCost(leftPlan, rightPlan);
    return intersectionNode;
}

PhysicalPlanNode* PhysicalPlan::createDifferencePlan(PhysicalPlanNode* leftPlan, PhysicalPlanNode* rightPlan) {
    PhysicalPlanNode* differenceNode = new PhysicalPlanNode();
    differenceNode->type = PlanNodeType::Difference;
    differenceNode->leftChild = leftPlan;
    differenceNode->rightChild = rightPlan;
    differenceNode->estimatedCost = CostEstimator::estimateDifferenceCost(leftPlan, rightPlan);
    return differenceNode;
}

PhysicalPlanNode* PhysicalPlan::createDistinctPlan(PhysicalPlanNode* inputPlan) {
    PhysicalPlanNode* distinctNode = new PhysicalPlanNode();
    distinctNode->type = PlanNodeType::Distinct;
    distinctNode->input = inputPlan;
    distinctNode->estimatedCost = CostEstimator::estimateDistinctCost(inputPlan);
    return distinctNode;
}

PhysicalPlanNode* PhysicalPlan::createDeletePlan(Table* table, const FilterCondition& condition) {
    PhysicalPlanNode* deleteNode = new PhysicalPlanNode();
    deleteNode->type = PlanNodeType::Delete;
    deleteNode->table = table;
    deleteNode->filterCondition = condition;
    deleteNode->estimatedCost = CostEstimator::estimateDeleteCost(table, condition);
    return deleteNode;
}

PhysicalPlanNode* PhysicalPlan::createUpdatePlan(Table* table, const UpdateSet& updateSet, const FilterCondition& condition) {
    PhysicalPlanNode* updateNode = new PhysicalPlanNode();
    updateNode->type = PlanNodeType::Update;
    updateNode->table = table;
    updateNode->updateSet = updateSet;
    updateNode->filterCondition = condition;
    updateNode->estimatedCost = CostEstimator::estimateUpdateCost(table, updateSet, condition);
    return updateNode;
}