#include <iostream>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <limits>
#include <algorithm>

class Transaction {
public:
    int id;
    unsigned long long start_ts;
    unsigned long long commit_ts;

    Transaction(int id, unsigned long long start_ts) : id(id), start_ts(start_ts), commit_ts(0) {}
};

class DataItem {
public:
    int id;
    unsigned long long read_ts;
    unsigned long long write_ts;
    int value;

    DataItem(int id, int value) : id(id), read_ts(0), write_ts(0), value(value) {}
};

class TimestampOrdering {
private:
    std::unordered_map<int, DataItem*> data_items;
    std::unordered_map<int, Transaction*> active_transactions;
    std::mutex mtx;

public:
    TimestampOrdering() {}

    void begin_transaction(int transaction_id, unsigned long long timestamp) {
        std::lock_guard<std::mutex> lock(mtx);
        if (active_transactions.find(transaction_id) == active_transactions.end()) {
            active_transactions[transaction_id] = new Transaction(transaction_id, timestamp);
            std::cout << "Transaction " << transaction_id << " started at " << timestamp << "\n";
        }
    }

    bool read(int transaction_id, int data_item_id) {
        std::lock_guard<std::mutex> lock(mtx);

        if (active_transactions.find(transaction_id) == active_transactions.end()) {
            std::cout << "Transaction " << transaction_id << " not found.\n";
            return false;
        }

        Transaction* txn = active_transactions[transaction_id];
        if (data_items.find(data_item_id) == data_items.end()) {
            std::cout << "Data item " << data_item_id << " not found.\n";
            return false;
        }

        DataItem* item = data_items[data_item_id];

        if (txn->start_ts < item->write_ts) {
            std::cout << "Transaction " << transaction_id << " aborted due to write-read conflict on item " << data_item_id << "\n";
            abort_transaction(transaction_id);
            return false;
        }

        item->read_ts = std::max(item->read_ts, txn->start_ts);
        std::cout << "Transaction " << transaction_id << " reads value " << item->value << " from item " << data_item_id << "\n";
        return true;
    }

    bool write(int transaction_id, int data_item_id, int new_value) {
        std::lock_guard<std::mutex> lock(mtx);

        if (active_transactions.find(transaction_id) == active_transactions.end()) {
            std::cout << "Transaction " << transaction_id << " not found.\n";
            return false;
        }

        Transaction* txn = active_transactions[transaction_id];
        if (data_items.find(data_item_id) == data_items.end()) {
            data_items[data_item_id] = new DataItem(data_item_id, 0);  // Initialize data item if it doesn't exist
        }

        DataItem* item = data_items[data_item_id];

        if (txn->start_ts < item->read_ts || txn->start_ts < item->write_ts) {
            std::cout << "Transaction " << transaction_id << " aborted due to read/write conflict on item " << data_item_id << "\n";
            abort_transaction(transaction_id);
            return false;
        }

        item->write_ts = std::max(item->write_ts, txn->start_ts);
        item->value = new_value;
        std::cout << "Transaction " << transaction_id << " writes value " << new_value << " to item " << data_item_id << "\n";
        return true;
    }

    void commit_transaction(int transaction_id, unsigned long long timestamp) {
        std::lock_guard<std::mutex> lock(mtx);

        if (active_transactions.find(transaction_id) == active_transactions.end()) {
            std::cout << "Transaction " << transaction_id << " not found.\n";
            return;
        }

        Transaction* txn = active_transactions[transaction_id];
        txn->commit_ts = timestamp;
        std::cout << "Transaction " << transaction_id << " committed at " << timestamp << "\n";
        active_transactions.erase(transaction_id);
        delete txn;
    }

    void abort_transaction(int transaction_id) {
        std::lock_guard<std::mutex> lock(mtx);

        if (active_transactions.find(transaction_id) == active_transactions.end()) {
            std::cout << "Transaction " << transaction_id << " not found.\n";
            return;
        }

        std::cout << "Transaction " << transaction_id << " aborted.\n";
        active_transactions.erase(transaction_id);
    }

    void print_state() {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "Current Database State:\n";
        for (auto& [key, item] : data_items) {
            std::cout << "Item " << key << " -> Value: " << item->value << ", Read TS: " << item->read_ts << ", Write TS: " << item->write_ts << "\n";
        }
    }
};

int main() {
    TimestampOrdering ts_order;

    ts_order.begin_transaction(1, 100);
    ts_order.write(1, 1, 10);
    ts_order.read(1, 1);
    ts_order.commit_transaction(1, 150);

    ts_order.begin_transaction(2, 200);
    ts_order.read(2, 1);
    ts_order.write(2, 1, 20);
    ts_order.commit_transaction(2, 250);

    ts_order.begin_transaction(3, 300);
    ts_order.read(3, 1);
    ts_order.commit_transaction(3, 350);

    ts_order.print_state();

    return 0;
}