#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

class Row {
public:
    std::vector<std::string> data;

    Row(const std::vector<std::string>& rowData) : data(rowData) {}

    void displayRow() const {
        for (const auto& field : data) {
            std::cout << field << " | ";
        }
        std::cout << std::endl;
    }
};

class Page {
private:
    int pageSize;
    std::vector<std::shared_ptr<Row>> rows;
    int rowCount;

public:
    Page(int size) : pageSize(size), rowCount(0) {}

    bool insertRow(const std::shared_ptr<Row>& row) {
        if (rowCount >= pageSize) {
            return false;
        }
        rows.push_back(row);
        rowCount++;
        return true;
    }

    bool deleteRow(int rowIndex) {
        if (rowIndex < 0 || rowIndex >= rowCount) {
            return false;
        }
        rows.erase(rows.begin() + rowIndex);
        rowCount--;
        return true;
    }

    std::shared_ptr<Row> getRow(int rowIndex) const {
        if (rowIndex < 0 || rowIndex >= rowCount) {
            return nullptr;
        }
        return rows[rowIndex];
    }

    int getRowCount() const {
        return rowCount;
    }

    void displayPage() const {
        for (const auto& row : rows) {
            row->displayRow();
        }
    }
};

class Table {
private:
    std::unordered_map<int, std::shared_ptr<Page>> pages;
    int pageSize;
    int currentPageId;

public:
    Table(int size) : pageSize(size), currentPageId(0) {
        pages[currentPageId] = std::make_shared<Page>(pageSize);
    }

    bool insertRow(const std::vector<std::string>& rowData) {
        auto row = std::make_shared<Row>(rowData);
        if (!pages[currentPageId]->insertRow(row)) {
            currentPageId++;
            pages[currentPageId] = std::make_shared<Page>(pageSize);
            return pages[currentPageId]->insertRow(row);
        }
        return true;
    }

    bool deleteRow(int pageId, int rowIndex) {
        if (pages.find(pageId) == pages.end()) {
            return false;
        }
        return pages[pageId]->deleteRow(rowIndex);
    }

    std::shared_ptr<Row> getRow(int pageId, int rowIndex) const {
        if (pages.find(pageId) == pages.end()) {
            return nullptr;
        }
        return pages.at(pageId)->getRow(rowIndex);
    }

    void displayTable() const {
        for (const auto& [pageId, page] : pages) {
            std::cout << "Page " << pageId << ":" << std::endl;
            page->displayPage();
        }
    }

    int getRowCount() const {
        int totalRows = 0;
        for (const auto& [pageId, page] : pages) {
            totalRows += page->getRowCount();
        }
        return totalRows;
    }
};

class RowStore {
private:
    std::unordered_map<std::string, std::shared_ptr<Table>> tables;

public:
    void createTable(const std::string& tableName, int pageSize) {
        if (tables.find(tableName) != tables.end()) {
            std::cout << "Table already exists." << std::endl;
            return;
        }
        tables[tableName] = std::make_shared<Table>(pageSize);
        std::cout << "Table " << tableName << " created with page size " << pageSize << "." << std::endl;
    }

    void insertRow(const std::string& tableName, const std::vector<std::string>& rowData) {
        if (tables.find(tableName) == tables.end()) {
            std::cout << "Table does not exist." << std::endl;
            return;
        }
        tables[tableName]->insertRow(rowData);
    }

    void deleteRow(const std::string& tableName, int pageId, int rowIndex) {
        if (tables.find(tableName) == tables.end()) {
            std::cout << "Table does not exist." << std::endl;
            return;
        }
        if (tables[tableName]->deleteRow(pageId, rowIndex)) {
            std::cout << "Row deleted successfully." << std::endl;
        } else {
            std::cout << "Failed to delete row." << std::endl;
        }
    }

    void getRow(const std::string& tableName, int pageId, int rowIndex) {
        if (tables.find(tableName) == tables.end()) {
            std::cout << "Table does not exist." << std::endl;
            return;
        }
        auto row = tables[tableName]->getRow(pageId, rowIndex);
        if (row) {
            row->displayRow();
        } else {
            std::cout << "Row not found." << std::endl;
        }
    }

    void displayTable(const std::string& tableName) {
        if (tables.find(tableName) == tables.end()) {
            std::cout << "Table does not exist." << std::endl;
            return;
        }
        tables[tableName]->displayTable();
    }
};

int main() {
    RowStore rowStore;

    rowStore.createTable("Users", 3);
    rowStore.insertRow("Users", {"1", "Person1", "person1@website.com"});
    rowStore.insertRow("Users", {"2", "Person2", "person2@website.com"});
    rowStore.insertRow("Users", {"3", "Person3", "person3@website.com"});
    rowStore.insertRow("Users", {"4", "Person4", "person4@website.com"}); // Triggers new page creation

    std::cout << "Displaying 'Users' table:" << std::endl;
    rowStore.displayTable("Users");

    std::cout << "Fetching row (Page 0, Row 1):" << std::endl;
    rowStore.getRow("Users", 0, 1);

    std::cout << "Deleting row (Page 0, Row 1):" << std::endl;
    rowStore.deleteRow("Users", 0, 1);

    std::cout << "Displaying 'Users' table after deletion:" << std::endl;
    rowStore.displayTable("Users");

    return 0;
}