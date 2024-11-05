#include <iostream>
#include <cassert>
#include "../storage/disk_storage/data_file_manager.cpp"
#include "../storage/disk_storage/log_file_manager.cpp"
#include "../storage/buffer_manager.cpp"
#include "../storage/compression/lz4_compression.cpp"
#include "../storage/compression/snappy_compression.cpp"
#include "../storage/storage_formats/row_store.cpp"
#include "../storage/storage_formats/column_store.cpp"

void test_data_file_creation() {
    DataFileManager dfm;
    std::string filePath = "test_data_file.dat";
    
    // Test file creation
    bool created = dfm.createFile(filePath);
    assert(created && "Data file should be created successfully");
    
    // Test if file exists
    bool exists = dfm.fileExists(filePath);
    assert(exists && "Data file should exist after creation");

    // Clean up
    dfm.deleteFile(filePath);
    assert(!dfm.fileExists(filePath) && "Data file should be deleted");
    
    std::cout << "Data file creation test passed!" << std::endl;
}

void test_log_file_management() {
    LogFileManager lfm;
    std::string logPath = "test_log_file.log";

    // Test log file creation
    lfm.createLogFile(logPath);
    assert(lfm.logExists(logPath) && "Log file should be created");

    // Test writing and reading logs
    std::string logData = "Sample log data";
    lfm.writeLog(logData);
    std::string readData = lfm.readLog();
    assert(logData == readData && "Log data should match what was written");

    // Clean up
    lfm.deleteLogFile(logPath);
    assert(!lfm.logExists(logPath) && "Log file should be deleted");
    
    std::cout << "Log file management test passed!" << std::endl;
}

void test_buffer_manager() {
    BufferManager bm;
    bm.initialize(1024);  // 1KB buffer size
    
    // Test buffer allocation
    char* buffer = bm.allocateBuffer();
    assert(buffer != nullptr && "Buffer should be allocated");
    
    // Test buffer deallocation
    bm.deallocateBuffer(buffer);
    std::cout << "Buffer manager test passed!" << std::endl;
}

void test_lz4_compression() {
    LZ4Compression lz4;
    std::string originalData = "This is a test string for LZ4 compression.";
    
    // Test compression
    std::string compressedData = lz4.compress(originalData);
    assert(!compressedData.empty() && "Data should be compressed");

    // Test decompression
    std::string decompressedData = lz4.decompress(compressedData);
    assert(originalData == decompressedData && "Decompressed data should match the original");
    
    std::cout << "LZ4 compression test passed!" << std::endl;
}

void test_snappy_compression() {
    SnappyCompression snappy;
    std::string originalData = "Another test string for Snappy compression.";
    
    // Test compression
    std::string compressedData = snappy.compress(originalData);
    assert(!compressedData.empty() && "Data should be compressed");

    // Test decompression
    std::string decompressedData = snappy.decompress(compressedData);
    assert(originalData == decompressedData && "Decompressed data should match the original");

    std::cout << "Snappy compression test passed!" << std::endl;
}

void test_row_store() {
    RowStore rowStore;
    rowStore.initialize(100);  // Initialize with 100 rows
    
    // Test row insertion
    std::string rowData = "Row data";
    bool inserted = rowStore.insertRow(1, rowData);
    assert(inserted && "Row should be inserted successfully");
    
    // Test row retrieval
    std::string retrievedRow = rowStore.getRow(1);
    assert(retrievedRow == rowData && "Retrieved row should match inserted data");
    
    std::cout << "Row store test passed!" << std::endl;
}

void test_column_store() {
    ColumnStore columnStore;
    columnStore.initialize(50);  // Initialize with 50 columns
    
    // Test column insertion
    std::string columnData = "Column data";
    bool inserted = columnStore.insertColumn(1, columnData);
    assert(inserted && "Column should be inserted successfully");

    // Test column retrieval
    std::string retrievedColumn = columnStore.getColumn(1);
    assert(retrievedColumn == columnData && "Retrieved column should match inserted data");

    std::cout << "Column store test passed!" << std::endl;
}

int main() {
    // Run all tests
    std::cout << "Running storage engine tests..." << std::endl;

    test_data_file_creation();
    test_log_file_management();
    test_buffer_manager();
    test_lz4_compression();
    test_snappy_compression();
    test_row_store();
    test_column_store();

    std::cout << "All storage engine tests passed!" << std::endl;
    
    return 0;
}