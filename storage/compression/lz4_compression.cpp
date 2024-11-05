#include <lz4.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>

class LZ4Compressor {
public:
    LZ4Compressor(int compressionLevel = LZ4HC_CLEVEL_DEFAULT)
        : compressionLevel(compressionLevel) {}

    std::vector<char> compress(const std::vector<char>& inputData) const {
        int maxCompressedSize = LZ4_compressBound(inputData.size());
        std::vector<char> compressedData(maxCompressedSize);

        int compressedSize = LZ4_compress_HC(
            inputData.data(), compressedData.data(), inputData.size(), maxCompressedSize, compressionLevel);

        if (compressedSize <= 0) {
            throw std::runtime_error("Compression failed.");
        }

        compressedData.resize(compressedSize);
        return compressedData;
    }

    std::vector<char> decompress(const std::vector<char>& compressedData, int originalSize) const {
        std::vector<char> decompressedData(originalSize);

        int decompressedSize = LZ4_decompress_safe(
            compressedData.data(), decompressedData.data(), compressedData.size(), originalSize);

        if (decompressedSize < 0) {
            throw std::runtime_error("Decompression failed.");
        }

        return decompressedData;
    }

private:
    int compressionLevel;
};

class FileHandler {
public:
    static std::vector<char> readFile(const std::string& fileName) {
        std::ifstream file(fileName, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open file: " + fileName);
        }

        file.seekg(0, std::ios::end);
        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(fileSize);
        if (!file.read(buffer.data(), fileSize)) {
            throw std::runtime_error("Failed to read file: " + fileName);
        }

        return buffer;
    }

    static void writeFile(const std::string& fileName, const std::vector<char>& data) {
        std::ofstream file(fileName, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open file: " + fileName);
        }

        if (!file.write(data.data(), data.size())) {
            throw std::runtime_error("Failed to write to file: " + fileName);
        }
    }
};

class LZ4FileCompressor {
public:
    LZ4FileCompressor(int compressionLevel = LZ4HC_CLEVEL_DEFAULT)
        : compressor(compressionLevel) {}

    void compressFile(const std::string& inputFileName, const std::string& outputFileName) const {
        auto inputData = FileHandler::readFile(inputFileName);
        auto compressedData = compressor.compress(inputData);
        FileHandler::writeFile(outputFileName, compressedData);
    }

    void decompressFile(const std::string& compressedFileName, const std::string& decompressedFileName, int originalSize) const {
        auto compressedData = FileHandler::readFile(compressedFileName);
        auto decompressedData = compressor.decompress(compressedData, originalSize);
        FileHandler::writeFile(decompressedFileName, decompressedData);
    }

private:
    LZ4Compressor compressor;
};

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <compress|decompress> <input> <output> <original_size>\n";
        return 1;
    }

    std::string mode = argv[1];
    std::string inputFileName = argv[2];
    std::string outputFileName = argv[3];
    int originalSize = std::stoi(argv[4]);

    try {
        LZ4FileCompressor fileCompressor;

        if (mode == "compress") {
            fileCompressor.compressFile(inputFileName, outputFileName);
            std::cout << "File compressed successfully: " << outputFileName << '\n';
        } else if (mode == "decompress") {
            fileCompressor.decompressFile(inputFileName, outputFileName, originalSize);
            std::cout << "File decompressed successfully: " << outputFileName << '\n';
        } else {
            std::cerr << "Invalid mode: " << mode << '\n';
            return 1;
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}