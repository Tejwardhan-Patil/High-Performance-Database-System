#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <stdexcept>

// Forward declaration of classes
class Column;
class ColumnStore;

// Enumeration for data types
enum class DataType {
    INT,
    FLOAT,
    STRING
};

// Abstract base class for column types
class Column {
public:
    virtual ~Column() = default;
    virtual void appendValue(const std::string& value) = 0;
    virtual std::string getValue(size_t index) const = 0;
    virtual size_t size() const = 0;
    virtual DataType getType() const = 0;
};

// Template class for different types of columns
template <typename T>
class TypedColumn : public Column {
private:
    std::vector<T> data;
    DataType type;

public:
    TypedColumn(DataType dataType) : type(dataType) {}

    void appendValue(const std::string& value) override {
        if constexpr (std::is_same<T, int>::value) {
            data.push_back(std::stoi(value));
        } else if constexpr (std::is_same<T, float>::value) {
            data.push_back(std::stof(value));
        } else if constexpr (std::is_same<T, std::string>::value) {
            data.push_back(value);
        } else {
            throw std::runtime_error("Unsupported data type");
        }
    }

    std::string getValue(size_t index) const override {
        if (index >= data.size()) {
            throw std::out_of_range("Index out of range");
        }
        if constexpr (std::is_same<T, int>::value) {
            return std::to_string(data[index]);
        } else if constexpr (std::is_same<T, float>::value) {
            return std::to_string(data[index]);
        } else if constexpr (std::is_same<T, std::string>::value) {
            return data[index];
        } else {
            throw std::runtime_error("Unsupported data type");
        }
    }

    size_t size() const override {
        return data.size();
    }

    DataType getType() const override {
        return type;
    }
};

// Column store class that manages multiple columns
class ColumnStore {
private:
    std::unordered_map<std::string, std::shared_ptr<Column>> columns;

public:
    // Add a new column
    void addColumn(const std::string& name, DataType type) {
        if (columns.find(name) != columns.end()) {
            throw std::runtime_error("Column already exists");
        }

        switch (type) {
            case DataType::INT:
                columns[name] = std::make_shared<TypedColumn<int>>(type);
                break;
            case DataType::FLOAT:
                columns[name] = std::make_shared<TypedColumn<float>>(type);
                break;
            case DataType::STRING:
                columns[name] = std::make_shared<TypedColumn<std::string>>(type);
                break;
            default:
                throw std::runtime_error("Unsupported data type");
        }
    }

    // Append a value to a specific column
    void appendValue(const std::string& columnName, const std::string& value) {
        auto it = columns.find(columnName);
        if (it == columns.end()) {
            throw std::runtime_error("Column not found");
        }
        it->second->appendValue(value);
    }

    // Get a value from a specific column and row
    std::string getValue(const std::string& columnName, size_t rowIndex) const {
        auto it = columns.find(columnName);
        if (it == columns.end()) {
            throw std::runtime_error("Column not found");
        }
        return it->second->getValue(rowIndex);
    }

    // Get the number of rows in the column store (based on the first column)
    size_t rowCount() const {
        if (columns.empty()) {
            return 0;
        }
        return columns.begin()->second->size();
    }

    // Get the data type of a specific column
    DataType getColumnType(const std::string& columnName) const {
        auto it = columns.find(columnName);
        if (it == columns.end()) {
            throw std::runtime_error("Column not found");
        }
        return it->second->getType();
    }

    // Display the data in the column store
    void display() const {
        if (columns.empty()) {
            std::cout << "No columns in store" << std::endl;
            return;
        }

        // Display column names
        for (const auto& columnPair : columns) {
            std::cout << columnPair.first << "\t";
        }
        std::cout << std::endl;

        // Display data row by row
        size_t rowCount = this->rowCount();
        for (size_t row = 0; row < rowCount; ++row) {
            for (const auto& columnPair : columns) {
                std::cout << columnPair.second->getValue(row) << "\t";
            }
            std::cout << std::endl;
        }
    }
};

// Helper function to determine data type from string
DataType determineDataType(const std::string& typeStr) {
    if (typeStr == "INT") {
        return DataType::INT;
    } else if (typeStr == "FLOAT") {
        return DataType::FLOAT;
    } else if (typeStr == "STRING") {
        return DataType::STRING;
    } else {
        throw std::runtime_error("Invalid data type");
    }
}

// Usage of the column store
int main() {
    ColumnStore store;

    // Adding columns
    store.addColumn("ID", DataType::INT);
    store.addColumn("Name", DataType::STRING);
    store.addColumn("Score", DataType::FLOAT);

    // Inserting data
    store.appendValue("ID", "1");
    store.appendValue("Name", "Person1");
    store.appendValue("Score", "95.5");

    store.appendValue("ID", "2");
    store.appendValue("Name", "Person2");
    store.appendValue("Score", "87.3");

    store.appendValue("ID", "3");
    store.appendValue("Name", "Person3");
    store.appendValue("Score", "92.1");

    // Display the content of the store
    store.display();

    return 0;
}