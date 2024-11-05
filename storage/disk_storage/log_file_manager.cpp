#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <thread>
#include <condition_variable>

class LogRecord {
public:
    enum RecordType { INSERT, DELETE, UPDATE, COMMIT, ABORT };

    LogRecord(RecordType type, int txId, const std::string& data)
        : type_(type), txId_(txId), data_(data), timestamp_(std::chrono::system_clock::now()) {}

    std::string toString() const {
        return "TxID: " + std::to_string(txId_) + " Type: " + std::to_string(type_) + " Data: " + data_ + 
               " Timestamp: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                                 timestamp_.time_since_epoch()).count());
    }

    int getTransactionId() const { return txId_; }
    RecordType getType() const { return type_; }
    std::string getData() const { return data_; }

private:
    RecordType type_;
    int txId_;
    std::string data_;
    std::chrono::system_clock::time_point timestamp_;
};

class LogFileManager {
public:
    LogFileManager(const std::string& logFilePath)
        : logFilePath_(logFilePath), logFileStream_(logFilePath_, std::ios::app | std::ios::out) {
        if (!logFileStream_.is_open()) {
            throw std::runtime_error("Unable to open log file.");
        }
    }

    ~LogFileManager() {
        if (logFileStream_.is_open()) {
            logFileStream_.close();
        }
    }

    void logRecord(const LogRecord& record) {
        std::lock_guard<std::mutex> lock(logMutex_);
        logFileStream_ << record.toString() << std::endl;
        pendingLogs_.push_back(record);
    }

    void flush() {
        std::lock_guard<std::mutex> lock(logMutex_);
        logFileStream_.flush();
    }

    void recover() {
        std::lock_guard<std::mutex> lock(logMutex_);
        std::ifstream inputFile(logFilePath_);
        std::string line;
        while (std::getline(inputFile, line)) {
            std::cout << "Recovered log: " << line << std::endl;
        }
    }

    void commitTransaction(int txId) {
        std::lock_guard<std::mutex> lock(logMutex_);
        LogRecord commitRecord(LogRecord::COMMIT, txId, "Commit");
        logRecord(commitRecord);
        flush();
        std::cout << "Transaction " << txId << " committed." << std::endl;
    }

    void abortTransaction(int txId) {
        std::lock_guard<std::mutex> lock(logMutex_);
        LogRecord abortRecord(LogRecord::ABORT, txId, "Abort");
        logRecord(abortRecord);
        flush();
        std::cout << "Transaction " << txId << " aborted." << std::endl;
    }

    void checkpoint() {
        std::lock_guard<std::mutex> lock(logMutex_);
        LogRecord checkpointRecord(LogRecord::COMMIT, -1, "Checkpoint");
        logRecord(checkpointRecord);
        flush();
        std::cout << "Checkpoint created." << std::endl;
    }

    std::vector<LogRecord> getPendingLogs() const {
        std::lock_guard<std::mutex> lock(logMutex_);
        return pendingLogs_;
    }

private:
    std::string logFilePath_;
    std::ofstream logFileStream_;
    std::vector<LogRecord> pendingLogs_;
    mutable std::mutex logMutex_;
};

class TransactionManager {
public:
    TransactionManager(LogFileManager& logFileManager) : logFileManager_(logFileManager), nextTxId_(1) {}

    int beginTransaction() {
        std::lock_guard<std::mutex> lock(txMutex_);
        int txId = nextTxId_++;
        LogRecord beginRecord(LogRecord::INSERT, txId, "Begin");
        logFileManager_.logRecord(beginRecord);
        return txId;
    }

    void commit(int txId) {
        logFileManager_.commitTransaction(txId);
    }

    void abort(int txId) {
        logFileManager_.abortTransaction(txId);
    }

private:
    LogFileManager& logFileManager_;
    int nextTxId_;
    std::mutex txMutex_;
};

class LogFlusher {
public:
    LogFlusher(LogFileManager& logFileManager) : logFileManager_(logFileManager), stopFlushing_(false) {}

    void start() {
        flusherThread_ = std::thread(&LogFlusher::flushLogsPeriodically, this);
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(flusherMutex_);
            stopFlushing_ = true;
        }
        flusherThread_.join();
    }

private:
    void flushLogsPeriodically() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            {
                std::lock_guard<std::mutex> lock(flusherMutex_);
                if (stopFlushing_) break;
            }
            logFileManager_.flush();
            std::cout << "Logs flushed." << std::endl;
        }
    }

    LogFileManager& logFileManager_;
    std::thread flusherThread_;
    std::mutex flusherMutex_;
    bool stopFlushing_;
};

int main() {
    try {
        LogFileManager logManager("logfile.log");

        TransactionManager txManager(logManager);

        int tx1 = txManager.beginTransaction();
        logManager.logRecord(LogRecord(LogRecord::INSERT, tx1, "Insert data into table A"));
        logManager.logRecord(LogRecord(LogRecord::UPDATE, tx1, "Update table A"));

        txManager.commit(tx1);

        int tx2 = txManager.beginTransaction();
        logManager.logRecord(LogRecord(LogRecord::INSERT, tx2, "Insert data into table B"));
        txManager.abort(tx2);

        LogFlusher logFlusher(logManager);
        logFlusher.start();

        logManager.checkpoint();

        logFlusher.stop();

        logManager.recover();

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    return 0;
}