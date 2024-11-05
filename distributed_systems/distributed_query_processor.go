package distributed_query_processor

import (
	"context"
	"errors"
	"fmt"
	"log"
	"sync"
	"time"
)

// Query represents a database query.
type Query struct {
	ID        string
	Statement string
}

// Result represents the result of a query.
type Result struct {
	QueryID string
	Data    []interface{}
	Error   error
}

// Node represents a distributed node that can process queries.
type Node struct {
	ID   string
	Addr string
}

// QueryProcessor handles query distribution and result aggregation across multiple nodes.
type QueryProcessor struct {
	nodes []*Node
	mu    sync.Mutex
}

// NewQueryProcessor initializes a QueryProcessor with a list of nodes.
func NewQueryProcessor(nodes []*Node) *QueryProcessor {
	return &QueryProcessor{nodes: nodes}
}

// ExecuteQuery sends a query to multiple nodes, processes the results, and aggregates them.
func (qp *QueryProcessor) ExecuteQuery(ctx context.Context, query *Query) (*Result, error) {
	if len(qp.nodes) == 0 {
		return nil, errors.New("no nodes available for processing")
	}

	resultChan := make(chan *Result, len(qp.nodes))
	var wg sync.WaitGroup

	for _, node := range qp.nodes {
		wg.Add(1)
		go func(n *Node) {
			defer wg.Done()
			res := qp.sendQueryToNode(ctx, n, query)
			resultChan <- res
		}(node)
	}

	go func() {
		wg.Wait()
		close(resultChan)
	}()

	aggregatedResult := &Result{QueryID: query.ID, Data: make([]interface{}, 0)}
	for res := range resultChan {
		if res.Error != nil {
			log.Printf("Error from node %s: %v", res.QueryID, res.Error)
			continue
		}
		aggregatedResult.Data = append(aggregatedResult.Data, res.Data...)
	}

	return aggregatedResult, nil
}

// sendQueryToNode sends a query to a specific node and waits for the result.
func (qp *QueryProcessor) sendQueryToNode(ctx context.Context, node *Node, query *Query) *Result {
	// Create a channel to simulate a result that could take time to generate.
	resultChan := make(chan *Result, 1)

	// Simulate network latency and query processing asynchronously.
	go func() {
		time.Sleep(time.Duration(50+int(node.ID[0])) * time.Millisecond)
		resultChan <- &Result{
			QueryID: query.ID,
			Data:    []interface{}{fmt.Sprintf("Result from node %s", node.ID)},
		}
	}()

	select {
	case <-ctx.Done():
		// Handle context cancellation or timeout.
		fmt.Println("Query canceled")
		return nil
	case result := <-resultChan:
		return result
	}
}

// AddNode adds a new node to the query processor.
func (qp *QueryProcessor) AddNode(node *Node) {
	qp.mu.Lock()
	defer qp.mu.Unlock()
	qp.nodes = append(qp.nodes, node)
}

// RemoveNode removes a node from the query processor by its ID.
func (qp *QueryProcessor) RemoveNode(nodeID string) {
	qp.mu.Lock()
	defer qp.mu.Unlock()

	for i, node := range qp.nodes {
		if node.ID == nodeID {
			qp.nodes = append(qp.nodes[:i], qp.nodes[i+1:]...)
			return
		}
	}
}

// HealthCheck performs a health check on all nodes and removes any that are unresponsive.
func (qp *QueryProcessor) HealthCheck() {
	qp.mu.Lock()
	defer qp.mu.Unlock()

	healthyNodes := make([]*Node, 0)
	for _, node := range qp.nodes {
		if qp.pingNode(node) {
			healthyNodes = append(healthyNodes, node)
		} else {
			log.Printf("Node %s is unresponsive and will be removed", node.ID)
		}
	}
	qp.nodes = healthyNodes
}

// pingNode simulates a health check by pinging a node.
func (qp *QueryProcessor) pingNode(*Node) bool {
	// Simulating health check delay.
	time.Sleep(10 * time.Millisecond)
	return true
}

// mainQueryLoop is the entry point for processing queries in a continuous loop.
func (qp *QueryProcessor) mainQueryLoop(ctx context.Context, queries <-chan *Query, results chan<- *Result) {
	for {
		select {
		case <-ctx.Done():
			log.Println("Shutting down query processor loop.")
			return
		case query := <-queries:
			if query != nil {
				res, err := qp.ExecuteQuery(ctx, query)
				if err != nil {
					log.Printf("Failed to execute query %s: %v", query.ID, err)
					continue
				}
				results <- res
			}
		}
	}
}

// StartQueryProcessor starts a query processing loop.
func (qp *QueryProcessor) StartQueryProcessor(ctx context.Context) (chan *Query, chan *Result) {
	queryChan := make(chan *Query)
	resultChan := make(chan *Result)

	go qp.mainQueryLoop(ctx, queryChan, resultChan)

	return queryChan, resultChan
}

// Shutdown gracefully shuts down the query processor.
func (qp *QueryProcessor) Shutdown(ctx context.Context) error {
	// Perform cleanup operations if necessary.
	log.Println("Shutting down query processor.")
	return nil
}

// Usage of the distributed query processor.
func main() {
	nodes := []*Node{
		{ID: "1", Addr: "192.168.1.1"},
		{ID: "2", Addr: "192.168.1.2"},
		{ID: "3", Addr: "192.168.1.3"},
	}

	qp := NewQueryProcessor(nodes)
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	queryChan, resultChan := qp.StartQueryProcessor(ctx)

	go func() {
		queries := []*Query{
			{ID: "q1", Statement: "SELECT * FROM users"},
			{ID: "q2", Statement: "SELECT * FROM orders"},
		}
		for _, query := range queries {
			queryChan <- query
		}
	}()

	go func() {
		for result := range resultChan {
			log.Printf("Received result for query %s: %v", result.QueryID, result.Data)
		}
	}()

	// Simulate running for a while.
	time.Sleep(5 * time.Second)
	qp.Shutdown(ctx)
}
