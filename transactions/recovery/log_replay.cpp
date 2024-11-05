#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

// Enums for log types and transaction states
enum class LogType { INSERT, DELETE, UPDATE, COMMIT, ABORT };
enum class TxState { ACTIVE, COMMITTED, ABORTED };

// Transaction log structure
struct LogRecord {
    int txID;               // Transaction ID
    LogType logType;        // Type of log (INSERT, DELETE, UPDATE, etc)
    std::string tableName;  // Table name affected by the transaction
    std::string data;       // Data involved in the transaction (before or after)
    int lsn;                // Log sequence number for ordering
};

// Transaction class to manage states
class Transaction {
public:
    int txID;
    TxState state;
    
    Transaction(int id) : txID(id), state(TxState::ACTIVE) {}
    
    void commit() { state = TxState::COMMITTED; }
    void abort() { state = TxState::ABORTED; }
};

// Class to manage log replay
class LogReplay {
private:
    std::unordered_map<int, Transaction> transactions;   // Active transactions during recovery
    std::vector<LogRecord> logs;                         // List of logs to replay

    void applyInsert(const LogRecord& log) {
        std::cout << "Inserting data: " << log.data << " into table " << log.tableName << std::endl;
    }

    void applyDelete(const LogRecord& log) {
        std::cout << "Deleting data: " << log.data << " from table " << log.tableName << std::endl;
    }

    void applyUpdate(const LogRecord& log) {
        std::cout << "Updating data: " << log.data << " in table " << log.tableName << std::endl;
    }

    void processCommit(int txID) {
        if (transactions.find(txID) != transactions.end()) {
            transactions[txID].commit();
            std::cout << "Transaction " << txID << " committed" << std::endl;
        }
    }

    void processAbort(int txID) {
        if (transactions.find(txID) != transactions.end()) {
            transactions[txID].abort();
            std::cout << "Transaction " << txID << " aborted" << std::endl;
        }
    }

public:
    LogReplay(const std::vector<LogRecord>& logRecords) : logs(logRecords) {}

    // Replays the logs in order
    void replayLogs() {
        for (const auto& log : logs) {
            // Start a new transaction if it doesn't exist
            if (transactions.find(log.txID) == transactions.end()) {
                transactions[log.txID] = Transaction(log.txID);
            }

            // Apply log based on type
            switch (log.logType) {
                case LogType::INSERT:
                    applyInsert(log);
                    break;
                case LogType::DELETE:
                    applyDelete(log);
                    break;
                case LogType::UPDATE:
                    applyUpdate(log);
                    break;
                case LogType::COMMIT:
                    processCommit(log.txID);
                    break;
                case LogType::ABORT:
                    processAbort(log.txID);
                    break;
                default:
                    throw std::runtime_error("Unknown log type");
            }
        }
    }
};

// Reads logs from a file
std::vector<LogRecord> readLogsFromFile(const std::string& filename) {
    std::vector<LogRecord> logRecords;
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open log file");
    }

    int txID, logType, lsn;
    std::string tableName, data;

    // Parse each log entry
    while (file >> txID >> logType >> tableName >> data >> lsn) {
        logRecords.push_back({txID, static_cast<LogType>(logType), tableName, data, lsn});
    }

    file.close();
    return logRecords;
}

int main() {
    try {
        // Log file
        std::string logFile = "logs.txt";
        
        // Read logs from file
        std::vector<LogRecord> logRecords = readLogsFromFile(logFile);

        // Instantiate log replay object
        LogReplay logReplay(logRecords);

        // Start replaying logs
        logReplay.replayLogs();
    } catch (const std::exception& e) {
        std::cerr << "Error during recovery: " << e.what() << std::endl;
    }

    return 0;
}