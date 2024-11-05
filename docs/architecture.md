# System Architecture Overview

## 1. Core Components

The system is built with several core components that work together to provide a high-performance database.

### Storage Engine

- **Disk Storage**: Manages on-disk storage, including data and log files.
  - *data_file_manager.cpp*: Manages the creation, deletion, and space management of data files.
  - *log_file_manager.cpp*: Manages Write-Ahead Logs (WAL) for durability.
- **Buffer Manager**: Manages in-memory data pages and caching.
- **Compression**: Implements LZ4 and Snappy compression algorithms to save storage space.
- **Storage Formats**: Supports both row-based and column-based storage formats.
  - *row_store.cpp*: Row-oriented storage.
  - *column_store.cpp*: Column-oriented storage.

### Indexing

- Supports multiple indexing techniques for faster data retrieval.
  - *b_tree_index.cpp*: B-tree indexing for balanced tree structure.
  - *hash_index.cpp*: Hash-based indexing for quick lookups.
  - *trie_index.cpp*: Trie-based indexing for prefix searches.
  - *r_tree.cpp*: R-tree indexing for spatial data.

### Query Processor

- **SQL/NoSQL Parsing**: Parses SQL and NoSQL queries.
- **Query Optimization**: Optimizes query execution plans.
- **Query Execution**: Executes optimized query plans.

### Transaction Management

- Ensures ACID properties through concurrency control, logging, and recovery mechanisms.
  - *two_phase_locking.cpp*: Two-phase locking for isolation.
  - *write_ahead_log.cpp*: WAL for durability.

### Networking and Distributed Systems

- Handles client-server interactions, RPC communication, load balancing, and distributed data management.
  - *rpc_server.go* and *rpc_client.go*: Manage RPC communication.
  - *raft.go*: Implements the Raft consensus algorithm for distributed consistency.

### Caching

- Implements LRU and LFU caching strategies to improve query performance.

### Security

- Includes authentication, encryption, and auditing mechanisms.
  - *auth_server.go*: Authentication server.
  - *encryption_at_rest.cpp*: Encryption for data storage.

---

## 2. Data Flow

Data flows from client applications through the networking layer, where queries are parsed and optimized before being executed on the storage engine. Indexes and caches are used to speed up data retrieval, while transactions ensure ACID properties. Data is distributed and replicated across nodes for high availability, with consensus algorithms ensuring consistency.

---

## 3. Distributed Architecture

The system supports distributed data storage and query processing, sharding data across multiple nodes and ensuring consistency with Raft or Paxos algorithms. Load balancing and distributed caching are used to handle scaling demands.
