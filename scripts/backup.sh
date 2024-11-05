#!/bin/bash

# Set variables for backup
BACKUP_DIR="/var/backups/db"        
DB_DIR="/var/lib/database"            
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")    
LOG_FILE="/var/log/db_backup.log"     
RETENTION_DAYS=7                        
BACKUP_FILE="$BACKUP_DIR/db_backup_$TIMESTAMP.tar.gz" 

# Create backup directory if it doesn't exist
mkdir -p "$BACKUP_DIR"

# Function to log messages
log_message() {
    echo "$(date +"%Y-%m-%d %H:%M:%S") : $1" >> "$LOG_FILE"
}

# Start backup process
log_message "Starting database backup process..."

# Check if database directory exists
if [ ! -d "$DB_DIR" ]; then
    log_message "ERROR: Database directory $DB_DIR does not exist."
    exit 1
fi

# Perform the backup using tar and compress with gzip
tar -czf "$BACKUP_FILE" -C "$DB_DIR" .
if [ $? -eq 0 ]; then
    log_message "Backup successful. Backup file created: $BACKUP_FILE"
else
    log_message "ERROR: Failed to create backup."
    exit 1
fi

# Remove old backups older than retention days
find "$BACKUP_DIR" -type f -name "db_backup_*.tar.gz" -mtime +$RETENTION_DAYS -exec rm -f {} \;
if [ $? -eq 0 ]; then
    log_message "Old backups removed successfully."
else
    log_message "ERROR: Failed to remove old backups."
fi

# Backup process completed
log_message "Database backup process completed."

exit 0