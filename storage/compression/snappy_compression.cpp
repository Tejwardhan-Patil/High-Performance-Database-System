#include <snappy.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>

class SnappyCompression {
public:
    // Constructor to initialize paths
    SnappyCompression(const std::string &inputFilePath, const std::string &outputFilePath)
        : inputFilePath(inputFilePath), outputFilePath(outputFilePath) {}

    // Compresses the file and stores the compressed content in output file
    bool compressFile() {
        std::ifstream inputFile(inputFilePath, std::ios::binary);
        if (!inputFile.is_open()) {
            std::cerr << "Failed to open input file: " << inputFilePath << std::endl;
            return false;
        }

        std::vector<char> inputBuffer((std::istreambuf_iterator<char>(inputFile)),
                                      std::istreambuf_iterator<char>());
        inputFile.close();

        std::string compressedBuffer;
        auto start = std::chrono::high_resolution_clock::now();
        snappy::Compress(inputBuffer.data(), inputBuffer.size(), &compressedBuffer);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> compressionTime = end - start;

        std::cout << "Compression took: " << compressionTime.count() << " seconds" << std::endl;

        std::ofstream outputFile(outputFilePath, std::ios::binary);
        if (!outputFile.is_open()) {
            std::cerr << "Failed to open output file: " << outputFilePath << std::endl;
            return false;
        }
        outputFile.write(compressedBuffer.data(), compressedBuffer.size());
        outputFile.close();

        std::cout << "File compressed successfully!" << std::endl;
        return true;
    }

    // Decompresses the file and stores the decompressed content in output file
    bool decompressFile() {
        std::ifstream inputFile(outputFilePath, std::ios::binary);
        if (!inputFile.is_open()) {
            std::cerr << "Failed to open compressed file: " << outputFilePath << std::endl;
            return false;
        }

        std::vector<char> compressedBuffer((std::istreambuf_iterator<char>(inputFile)),
                                           std::istreambuf_iterator<char>());
        inputFile.close();

        size_t uncompressedLength;
        if (!snappy::GetUncompressedLength(compressedBuffer.data(), compressedBuffer.size(), &uncompressedLength)) {
            std::cerr << "Failed to get uncompressed length!" << std::endl;
            return false;
        }

        std::string decompressedBuffer;
        decompressedBuffer.resize(uncompressedLength);

        auto start = std::chrono::high_resolution_clock::now();
        if (!snappy::RawUncompress(compressedBuffer.data(), compressedBuffer.size(), &decompressedBuffer[0])) {
            std::cerr << "Decompression failed!" << std::endl;
            return false;
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> decompressionTime = end - start;

        std::cout << "Decompression took: " << decompressionTime.count() << " seconds" << std::endl;

        std::ofstream outputFile("decompressed_" + inputFilePath, std::ios::binary);
        if (!outputFile.is_open()) {
            std::cerr << "Failed to open output file: decompressed_" << inputFilePath << std::endl;
            return false;
        }
        outputFile.write(decompressedBuffer.data(), decompressedBuffer.size());
        outputFile.close();

        std::cout << "File decompressed successfully!" << std::endl;
        return true;
    }

    // Verify the compressed and decompressed content matches
    bool verifyCompression() {
        std::ifstream originalFile(inputFilePath, std::ios::binary);
        if (!originalFile.is_open()) {
            std::cerr << "Failed to open original file for verification: " << inputFilePath << std::endl;
            return false;
        }
        std::vector<char> originalBuffer((std::istreambuf_iterator<char>(originalFile)),
                                         std::istreambuf_iterator<char>());
        originalFile.close();

        std::ifstream decompressedFile("decompressed_" + inputFilePath, std::ios::binary);
        if (!decompressedFile.is_open()) {
            std::cerr << "Failed to open decompressed file for verification: decompressed_" << inputFilePath << std::endl;
            return false;
        }
        std::vector<char> decompressedBuffer((std::istreambuf_iterator<char>(decompressedFile)),
                                             std::istreambuf_iterator<char>());
        decompressedFile.close();

        if (originalBuffer == decompressedBuffer) {
            std::cout << "Verification succeeded: decompressed content matches original!" << std::endl;
            return true;
        } else {
            std::cerr << "Verification failed: decompressed content does not match original!" << std::endl;
            return false;
        }
    }

private:
    std::string inputFilePath;
    std::string outputFilePath;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input file> <compressed output file>" << std::endl;
        return 1;
    }

    SnappyCompression compressor(argv[1], argv[2]);

    // Compress the input file
    if (!compressor.compressFile()) {
        return 1;
    }

    // Decompress the output file
    if (!compressor.decompressFile()) {
        return 1;
    }

    // Verify that the decompressed file matches the original
    if (!compressor.verifyCompression()) {
        return 1;
    }

    return 0;
}