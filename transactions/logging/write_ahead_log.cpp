#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <vector>
#include <chrono>
#include <thread>
#include <filesystem>

// Enum to represent the different types of log records.
enum class LogRecordType {
    BEGIN_TRANSACTION,
    COMMIT_TRANSACTION,
    ABORT_TRANSACTION,
    UPDATE
};

// Structure to represent a log record.
struct LogRecord {
    LogRecordType type;
    int transaction_id;
    int page_id;
    std::string old_data;
    std::string new_data;
    long long timestamp;

    LogRecord(LogRecordType type, int tx_id, int pg_id, const std::string& old_d, const std::string& new_d)
        : type(type), transaction_id(tx_id), page_id(pg_id), old_data(old_d), new_data(new_d) {
        timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    }

    std::string ToString() const {
        return std::to_string(static_cast<int>(type)) + "," + 
               std::to_string(transaction_id) + "," +
               std::to_string(page_id) + "," + old_data + "," + new_data + "," +
               std::to_string(timestamp) + "\n";
    }
};

// Class representing the Write-Ahead Log.
class WriteAheadLog {
public:
    WriteAheadLog(const std::string& log_file) : log_file_path(log_file) {
        std::lock_guard<std::mutex> lock(file_mutex);
        if (!std::filesystem::exists(log_file_path)) {
            std::ofstream log_file(log_file_path);
            log_file.close();
        }
    }

    // Write a log record.
    void WriteLog(const LogRecord& log_record) {
        std::lock_guard<std::mutex> lock(file_mutex);
        std::ofstream log_file(log_file_path, std::ios::app);
        if (log_file.is_open()) {
            log_file << log_record.ToString();
            log_file.close();
        }
    }

    // Read all log records from the log file.
    std::vector<LogRecord> ReadLogs() {
        std::lock_guard<std::mutex> lock(file_mutex);
        std::vector<LogRecord> logs;
        std::ifstream log_file(log_file_path);
        std::string line;

        while (std::getline(log_file, line)) {
            LogRecord record = ParseLogRecord(line);
            logs.push_back(record);
        }
        return logs;
    }

    // Remove the log file after a successful checkpoint.
    void ClearLogs() {
        std::lock_guard<std::mutex> lock(file_mutex);
        if (std::filesystem::exists(log_file_path)) {
            std::filesystem::remove(log_file_path);
        }
    }

private:
    std::string log_file_path;
    std::mutex file_mutex;

    // Parse a log record from a string.
    LogRecord ParseLogRecord(const std::string& log_line) {
        std::stringstream ss(log_line);
        std::vector<std::string> parts;
        std::string token;
        while (std::getline(ss, token, ',')) {
            parts.push_back(token);
        }

        LogRecordType type = static_cast<LogRecordType>(std::stoi(parts[0]));
        int tx_id = std::stoi(parts[1]);
        int page_id = std::stoi(parts[2]);
        std::string old_data = parts[3];
        std::string new_data = parts[4];
        long long timestamp = std::stoll(parts[5]);

        return LogRecord(type, tx_id, page_id, old_data, new_data);
    }
};

// Function to simulate a transaction that generates logs.
void SimulateTransaction(WriteAheadLog& wal, int transaction_id) {
    std::cout << "Transaction " << transaction_id << " started.\n";
    wal.WriteLog(LogRecord(LogRecordType::BEGIN_TRANSACTION, transaction_id, -1, "", ""));

    // Simulate updates to pages.
    for (int i = 0; i < 5; ++i) {
        std::string old_data = "old_data_" + std::to_string(i);
        std::string new_data = "new_data_" + std::to_string(i);
        wal.WriteLog(LogRecord(LogRecordType::UPDATE, transaction_id, i, old_data, new_data));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    wal.WriteLog(LogRecord(LogRecordType::COMMIT_TRANSACTION, transaction_id, -1, "", ""));
    std::cout << "Transaction " << transaction_id << " committed.\n";
}

// Function to simulate recovery using the log file.
void RecoverFromLogs(WriteAheadLog& wal) {
    std::cout << "Recovering from logs...\n";
    std::vector<LogRecord> logs = wal.ReadLogs();

    for (const LogRecord& log : logs) {
        if (log.type == LogRecordType::BEGIN_TRANSACTION) {
            std::cout << "Recovering transaction: " << log.transaction_id << "\n";
        } else if (log.type == LogRecordType::UPDATE) {
            std::cout << "Replaying update for page: " << log.page_id << ", Transaction: " << log.transaction_id << "\n";
        } else if (log.type == LogRecordType::COMMIT_TRANSACTION) {
            std::cout << "Transaction " << log.transaction_id << " committed.\n";
        }
    }
    wal.ClearLogs();
    std::cout << "Recovery complete.\n";
}

int main() {
    WriteAheadLog wal("wal_log.txt");

    // Simulate two transactions.
    std::thread t1(SimulateTransaction, std::ref(wal), 1);
    std::thread t2(SimulateTransaction, std::ref(wal), 2);

    t1.join();
    t2.join();

    // Simulate a system crash and recovery.
    std::cout << "Simulating system crash...\n";
    RecoverFromLogs(wal);

    return 0;
}