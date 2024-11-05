#!/bin/bash

# Exit script on any error
set -e

# Load environment configurations
echo "Loading environment configuration..."
CONFIG_FILE="env_configs/config.prod.yaml"
if [ "$1" == "dev" ]; then
  CONFIG_FILE="env_configs/config.dev.yaml"
fi
echo "Using configuration file: $CONFIG_FILE"

# Build the project
echo "Building project components..."
./scripts/build.sh

# Define target environment
TARGET_ENVIRONMENT="production"
if [ "$1" == "dev" ]; then
  TARGET_ENVIRONMENT="development"
fi
echo "Deploying to $TARGET_ENVIRONMENT environment..."

# Check if Docker is running
echo "Checking Docker status..."
if ! docker info > /dev/null 2>&1; then
  echo "Docker is not running. Please start Docker and try again."
  exit 1
fi

# Pull the latest Docker image
echo "Pulling the latest Docker image..."
docker pull website.com/high-performance-db:latest

# Start the database system using Docker Compose
echo "Starting the database system..."
docker-compose -f docker-compose.yml up -d

# Apply any necessary database migrations
echo "Applying database migrations..."
docker exec -it db_container_name ./scripts/migrate.sh

# Run health checks
echo "Running health checks..."
docker exec -it db_container_name ./scripts/health_check.sh

# Notify the user of successful deployment
echo "Deployment to $TARGET_ENVIRONMENT completed successfully!"