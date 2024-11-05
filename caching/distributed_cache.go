package main

import (
	"errors"
	"fmt"
	"log"
	"net/http"
	"sync"
	"time"
)

// Node represents a cache node in the distributed system
type Node struct {
	Address string
	Client  *http.Client
}

// DistributedCache represents the main cache with multiple nodes
type DistributedCache struct {
	mu     sync.RWMutex
	data   map[string]string
	nodes  []Node
	leader int
}

// NewDistributedCache initializes a distributed cache
func NewDistributedCache(nodes []Node) *DistributedCache {
	cache := &DistributedCache{
		data:   make(map[string]string),
		nodes:  nodes,
		leader: 0, // Initially, the first node is the leader
	}
	go cache.monitorLeader()
	return cache
}

// monitorLeader checks the status of the leader and elects a new one
func (cache *DistributedCache) monitorLeader() {
	for {
		time.Sleep(5 * time.Second)
		cache.mu.Lock()
		leader := cache.nodes[cache.leader]
		if !cache.pingNode(leader) {
			fmt.Println("Leader is down, electing a new leader...")
			cache.electNewLeader()
		}
		cache.mu.Unlock()
	}
}

// electNewLeader selects a new leader from available nodes
func (cache *DistributedCache) electNewLeader() {
	for i, node := range cache.nodes {
		if cache.pingNode(node) {
			cache.leader = i
			fmt.Printf("New leader elected: %s\n", node.Address)
			return
		}
	}
	fmt.Println("No available leaders found")
}

// pingNode checks if a node is reachable
func (cache *DistributedCache) pingNode(node Node) bool {
	_, err := http.Get(node.Address + "/ping")
	return err == nil
}

// Set stores a key-value pair in the distributed cache
func (cache *DistributedCache) Set(key, value string) error {
	cache.mu.Lock()
	defer cache.mu.Unlock()

	// Set value in local cache
	cache.data[key] = value

	// Replicate the change to other nodes
	for i, node := range cache.nodes {
		if i != cache.leader {
			go cache.replicateSet(node, key, value)
		}
	}

	return nil
}

// replicateSet sends a SET request to another node
func (cache *DistributedCache) replicateSet(node Node, key, value string) {
	url := fmt.Sprintf("%s/set?key=%s&value=%s", node.Address, key, value)
	_, err := http.Get(url)
	if err != nil {
		log.Printf("Failed to replicate set on node %s: %v\n", node.Address, err)
	}
}

// Get retrieves a value from the distributed cache
func (cache *DistributedCache) Get(key string) (string, error) {
	cache.mu.RLock()
	defer cache.mu.RUnlock()

	// Check local cache first
	value, ok := cache.data[key]
	if ok {
		return value, nil
	}

	// If not found, try to fetch from other nodes
	for _, node := range cache.nodes {
		val, err := cache.fetchFromNode(node, key)
		if err == nil {
			return val, nil
		}
	}

	return "", errors.New("key not found")
}

// fetchFromNode attempts to get a value from a specific node
func (cache *DistributedCache) fetchFromNode(node Node, key string) (string, error) {
	url := fmt.Sprintf("%s/get?key=%s", node.Address, key)
	resp, err := http.Get(url)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()

	var value string
	if _, err := fmt.Fscanf(resp.Body, "%s", &value); err != nil {
		return "", err
	}
	return value, nil
}

// Delete removes a key-value pair from the distributed cache
func (cache *DistributedCache) Delete(key string) error {
	cache.mu.Lock()
	defer cache.mu.Unlock()

	// Delete from local cache
	delete(cache.data, key)

	// Replicate the deletion to other nodes
	for i, node := range cache.nodes {
		if i != cache.leader {
			go cache.replicateDelete(node, key)
		}
	}

	return nil
}

// replicateDelete sends a DELETE request to another node
func (cache *DistributedCache) replicateDelete(node Node, key string) {
	url := fmt.Sprintf("%s/delete?key=%s", node.Address, key)
	_, err := http.Get(url)
	if err != nil {
		log.Printf("Failed to replicate delete on node %s: %v\n", node.Address, err)
	}
}

// ServeHTTP allows the cache to respond to HTTP requests
func (cache *DistributedCache) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	switch r.URL.Path {
	case "/ping":
		w.Write([]byte("pong"))
	case "/set":
		key := r.URL.Query().Get("key")
		value := r.URL.Query().Get("value")
		cache.Set(key, value)
		w.Write([]byte("OK"))
	case "/get":
		key := r.URL.Query().Get("key")
		value, err := cache.Get(key)
		if err != nil {
			http.Error(w, "Not Found", http.StatusNotFound)
			return
		}
		w.Write([]byte(value))
	case "/delete":
		key := r.URL.Query().Get("key")
		cache.Delete(key)
		w.Write([]byte("OK"))
	default:
		http.Error(w, "Invalid endpoint", http.StatusNotFound)
	}
}

func main() {
	nodes := []Node{
		{Address: "http://localhost:8001", Client: &http.Client{}},
		{Address: "http://localhost:8002", Client: &http.Client{}},
		{Address: "http://localhost:8003", Client: &http.Client{}},
	}

	cache := NewDistributedCache(nodes)

	http.Handle("/", cache)
	log.Fatal(http.ListenAndServe(":8000", nil))
}
