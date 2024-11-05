#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <filesystem>
#include <cstdio>

namespace fs = std::filesystem;

class DataFileManager {
private:
    std::string baseDirectory;
    std::unordered_map<std::string, std::fstream> openFiles;
    std::mutex fileMutex;

public:
    DataFileManager(const std::string& baseDir) : baseDirectory(baseDir) {
        if (!fs::exists(baseDir)) {
            fs::create_directory(baseDir);
        }
    }

    ~DataFileManager() {
        for (auto& [fileName, fileStream] : openFiles) {
            if (fileStream.is_open()) {
                fileStream.close();
            }
        }
    }

    bool createFile(const std::string& fileName) {
        std::lock_guard<std::mutex> lock(fileMutex);
        std::string filePath = baseDirectory + "/" + fileName;
        std::ofstream file(filePath);
        if (!file) {
            std::cerr << "Error creating file: " << fileName << std::endl;
            return false;
        }
        file.close();
        return true;
    }

    bool deleteFile(const std::string& fileName) {
        std::lock_guard<std::mutex> lock(fileMutex);
        std::string filePath = baseDirectory + "/" + fileName;
        if (fs::exists(filePath)) {
            return fs::remove(filePath);
        }
        return false;
    }

    bool openFile(const std::string& fileName) {
        std::lock_guard<std::mutex> lock(fileMutex);
        std::string filePath = baseDirectory + "/" + fileName;
        if (openFiles.find(fileName) == openFiles.end()) {
            std::fstream file(filePath, std::ios::in | std::ios::out | std::ios::binary);
            if (!file.is_open()) {
                std::cerr << "Error opening file: " << fileName << std::endl;
                return false;
            }
            openFiles[fileName] = std::move(file);
        }
        return true;
    }

    bool closeFile(const std::string& fileName) {
        std::lock_guard<std::mutex> lock(fileMutex);
        if (openFiles.find(fileName) != openFiles.end()) {
            if (openFiles[fileName].is_open()) {
                openFiles[fileName].close();
            }
            openFiles.erase(fileName);
            return true;
        }
        return false;
    }

    bool writeFile(const std::string& fileName, const std::vector<char>& data, std::streampos offset = 0) {
        std::lock_guard<std::mutex> lock(fileMutex);
        if (openFiles.find(fileName) != openFiles.end()) {
            openFiles[fileName].seekp(offset, std::ios::beg);
            openFiles[fileName].write(data.data(), data.size());
            return true;
        }
        std::cerr << "File not open: " << fileName << std::endl;
        return false;
    }

    bool readFile(const std::string& fileName, std::vector<char>& buffer, std::streampos offset, std::streamsize length) {
        std::lock_guard<std::mutex> lock(fileMutex);
        if (openFiles.find(fileName) != openFiles.end()) {
            openFiles[fileName].seekg(offset, std::ios::beg);
            buffer.resize(length);
            openFiles[fileName].read(buffer.data(), length);
            return true;
        }
        std::cerr << "File not open: " << fileName << std::endl;
        return false;
    }

    std::streamsize getFileSize(const std::string& fileName) {
        std::lock_guard<std::mutex> lock(fileMutex);
        std::string filePath = baseDirectory + "/" + fileName;
        if (fs::exists(filePath)) {
            return fs::file_size(filePath);
        }
        return -1;
    }

    std::vector<std::string> listFiles() const {
        std::vector<std::string> files;
        for (const auto& entry : fs::directory_iterator(baseDirectory)) {
            if (fs::is_regular_file(entry.status())) {
                files.push_back(entry.path().filename().string());
            }
        }
        return files;
    }

    bool allocateSpace(const std::string& fileName, std::streamsize size) {
        std::lock_guard<std::mutex> lock(fileMutex);
        std::string filePath = baseDirectory + "/" + fileName;
        std::fstream file(filePath, std::ios::in | std::ios::out | std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "File not open: " << fileName << std::endl;
            return false;
        }
        file.seekp(0, std::ios::end);
        std::streamsize currentSize = file.tellp();
        if (currentSize < size) {
            file.seekp(size - 1, std::ios::beg);
            file.put('\0');
        }
        return true;
    }

    bool truncateFile(const std::string& fileName, std::streamsize newSize) {
        std::lock_guard<std::mutex> lock(fileMutex);
        std::string filePath = baseDirectory + "/" + fileName;
        if (fs::exists(filePath)) {
            std::fstream file(filePath, std::ios::in | std::ios::out | std::ios::binary);
            if (file.is_open()) {
                file.seekp(0, std::ios::end);
                std::streamsize currentSize = file.tellp();
                if (currentSize > newSize) {
                    fs::resize_file(filePath, newSize);
                }
                return true;
            }
        }
        return false;
    }

    void defragment() {
        std::lock_guard<std::mutex> lock(fileMutex);
        for (const auto& fileName : listFiles()) {
            std::string filePath = baseDirectory + "/" + fileName;
            std::vector<char> buffer;
            std::fstream file(filePath, std::ios::in | std::ios::out | std::ios::binary);
            if (file.is_open()) {
                file.seekg(0, std::ios::end);
                std::streamsize size = file.tellg();
                file.seekg(0, std::ios::beg);
                buffer.resize(size);
                file.read(buffer.data(), size);

                file.close();
                file.open(filePath, std::ios::out | std::ios::trunc | std::ios::binary);
                file.write(buffer.data(), buffer.size());
            }
        }
    }

    void printDirectoryInfo() const {
        for (const auto& fileName : listFiles()) {
            std::cout << "File: " << fileName << " Size: " << getFileSize(fileName) << " bytes" << std::endl;
        }
    }
};

// Usage
int main() {
    DataFileManager dfm("data_directory");
    
    dfm.createFile("test_file.dat");
    dfm.allocateSpace("test_file.dat", 1024);
    
    std::vector<char> data = {'H', 'e', 'l', 'l', 'o'};
    dfm.openFile("test_file.dat");
    dfm.writeFile("test_file.dat", data);
    
    std::vector<char> buffer;
    dfm.readFile("test_file.dat", buffer, 0, data.size());
    
    for (char c : buffer) {
        std::cout << c;
    }
    std::cout << std::endl;

    dfm.printDirectoryInfo();
    dfm.closeFile("test_file.dat");
    dfm.deleteFile("test_file.dat");

    return 0;
}