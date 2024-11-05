#!/bin/bash

# Set the database storage location and backup directory
DB_DATA_DIR="/var/lib/mydb/data"
BACKUP_DIR="/backups"
LOG_FILE="/var/log/db_restore.log"

# Function to log messages
log_message() {
    echo "$(date +'%Y-%m-%d %H:%M:%S') - $1" | tee -a $LOG_FILE
}

# Check if backup file is provided as an argument
if [ -z "$1" ]; then
    log_message "ERROR: No backup file specified."
    echo "Usage: $0 <backup-file>"
    exit 1
fi

BACKUP_FILE="$1"

# Check if the specified backup file exists
if [ ! -f "$BACKUP_FILE" ]; then
    log_message "ERROR: Backup file $BACKUP_FILE does not exist."
    exit 1
fi

log_message "Starting database restore from backup: $BACKUP_FILE"

# Stop the database service before restoring
log_message "Stopping the database service..."
systemctl stop mydb

if [ $? -ne 0 ]; then
    log_message "ERROR: Failed to stop the database service."
    exit 1
fi

# Clear the current database data directory
log_message "Clearing the current database data directory: $DB_DATA_DIR"
rm -rf $DB_DATA_DIR/*
if [ $? -ne 0 ]; then
    log_message "ERROR: Failed to clear the database data directory."
    exit 1
fi

# Extract the backup into the database data directory
log_message "Restoring backup to $DB_DATA_DIR"
tar -xzf $BACKUP_FILE -C $DB_DATA_DIR
if [ $? -ne 0 ]; then
    log_message "ERROR: Failed to extract backup file."
    exit 1
fi

# Set the correct permissions for the restored files
log_message "Setting correct permissions for the restored files..."
chown -R mydbuser:mydbgroup $DB_DATA_DIR
if [ $? -ne 0 ]; then
    log_message "ERROR: Failed to set file permissions."
    exit 1
fi

# Start the database service after restoring
log_message "Starting the database service..."
systemctl start mydb
if [ $? -ne 0 ]; then
    log_message "ERROR: Failed to start the database service."
    exit 1
fi

log_message "Database restore completed successfully."

exit 0