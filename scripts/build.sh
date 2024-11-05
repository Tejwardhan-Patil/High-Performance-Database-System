#!/bin/bash

# Stop the script if any command fails
set -e

# Paths to the main components
STORAGE_PATH="./storage"
INDEXING_PATH="./indexing"
QUERY_PROCESSOR_PATH="./query_processor"
TRANSACTION_PATH="./transactions"
NETWORKING_PATH="./networking"
DISTRIBUTED_SYSTEMS_PATH="./distributed_systems"
CACHING_PATH="./caching"
CONFIG_PATH="./config"
SECURITY_PATH="./security"
TESTS_PATH="./tests"

# Build function for C++ components
build_cpp_component() {
    echo "Building C++ component: $1"
    cd $1
    mkdir -p build
    cd build
    cmake ..
    make -j$(nproc)
    cd ../../
}

# Build function for Go components
build_go_component() {
    echo "Building Go component: $1"
    cd $1
    go build .
    cd ../../
}

# Start building process
echo "Starting the build process for High-Performance Database System..."

# Build Storage Engine
build_cpp_component $STORAGE_PATH

# Build Indexing
build_cpp_component $INDEXING_PATH

# Build Query Processor
build_cpp_component $QUERY_PROCESSOR_PATH

# Build Transaction Management
build_cpp_component $TRANSACTION_PATH

# Build Networking
build_go_component $NETWORKING_PATH

# Build Distributed Systems
build_go_component $DISTRIBUTED_SYSTEMS_PATH

# Build Caching
build_cpp_component $CACHING_PATH

# Build Security
build_cpp_component $SECURITY_PATH

# Build Configuration Management Tools (in Go)
build_go_component $CONFIG_PATH

# Run tests if specified
if [[ "$1" == "--test" ]]; then
    echo "Running unit tests..."
    build_cpp_component $TESTS_PATH
    echo "Tests completed successfully."
fi

echo "Build process completed successfully!"