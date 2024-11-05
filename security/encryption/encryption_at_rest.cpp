#include <iostream>
#include <fstream>
#include <string>
#include <openssl/evp.h>
#include <openssl/rand.h>

const int KEY_SIZE = 32; // 256-bit AES key
const int IV_SIZE = 16;  // 128-bit IV

// Utility function to generate a random encryption key
bool generate_key(unsigned char *key, unsigned char *iv) {
    if (!RAND_bytes(key, KEY_SIZE) || !RAND_bytes(iv, IV_SIZE)) {
        std::cerr << "Failed to generate key or IV" << std::endl;
        return false;
    }
    return true;
}

// Function to encrypt data using AES-256-CBC
bool encrypt(const std::string &plaintext, const std::string &output_file, unsigned char *key, unsigned char *iv) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        std::cerr << "Failed to create cipher context" << std::endl;
        return false;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv) != 1) {
        std::cerr << "Failed to initialize encryption" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    std::ofstream outfile(output_file, std::ios::binary);
    if (!outfile.is_open()) {
        std::cerr << "Failed to open output file" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int block_size = EVP_CIPHER_block_size(EVP_aes_256_cbc());
    unsigned char *ciphertext = new unsigned char[plaintext.size() + block_size];
    int len;
    int ciphertext_len = 0;

    if (EVP_EncryptUpdate(ctx, ciphertext, &len, reinterpret_cast<const unsigned char *>(plaintext.c_str()), plaintext.size()) != 1) {
        std::cerr << "Failed to encrypt data" << std::endl;
        delete[] ciphertext;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len += len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
        std::cerr << "Failed to finalize encryption" << std::endl;
        delete[] ciphertext;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len += len;

    outfile.write(reinterpret_cast<char *>(ciphertext), ciphertext_len);
    delete[] ciphertext;
    outfile.close();

    EVP_CIPHER_CTX_free(ctx);
    return true;
}

// Function to decrypt data using AES-256-CBC
bool decrypt(const std::string &input_file, std::string &plaintext, unsigned char *key, unsigned char *iv) {
    std::ifstream infile(input_file, std::ios::binary);
    if (!infile.is_open()) {
        std::cerr << "Failed to open input file" << std::endl;
        return false;
    }

    infile.seekg(0, std::ios::end);
    std::streampos file_size = infile.tellg();
    infile.seekg(0, std::ios::beg);

    unsigned char *ciphertext = new unsigned char[file_size];
    infile.read(reinterpret_cast<char *>(ciphertext), file_size);
    infile.close();

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        std::cerr << "Failed to create cipher context" << std::endl;
        delete[] ciphertext;
        return false;
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv) != 1) {
        std::cerr << "Failed to initialize decryption" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        delete[] ciphertext;
        return false;
    }

    unsigned char *decrypted_text = new unsigned char[file_size];
    int len;
    int decrypted_text_len = 0;

    if (EVP_DecryptUpdate(ctx, decrypted_text, &len, ciphertext, file_size) != 1) {
        std::cerr << "Failed to decrypt data" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        delete[] ciphertext;
        delete[] decrypted_text;
        return false;
    }
    decrypted_text_len += len;

    if (EVP_DecryptFinal_ex(ctx, decrypted_text + len, &len) != 1) {
        std::cerr << "Failed to finalize decryption" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        delete[] ciphertext;
        delete[] decrypted_text;
        return false;
    }
    decrypted_text_len += len;

    plaintext.assign(reinterpret_cast<char *>(decrypted_text), decrypted_text_len);

    delete[] ciphertext;
    delete[] decrypted_text;
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

int main() {
    std::string data_to_encrypt = "Sensitive data that needs to be encrypted at rest.";

    unsigned char key[KEY_SIZE];
    unsigned char iv[IV_SIZE];

    // Generate key and IV
    if (!generate_key(key, iv)) {
        return 1;
    }

    // Encrypt the data and write to a file
    if (!encrypt(data_to_encrypt, "encrypted_data.bin", key, iv)) {
        std::cerr << "Encryption failed" << std::endl;
        return 1;
    }

    std::cout << "Data encrypted and saved to encrypted_data.bin" << std::endl;

    // Decrypt the data from the file
    std::string decrypted_data;
    if (!decrypt("encrypted_data.bin", decrypted_data, key, iv)) {
        std::cerr << "Decryption failed" << std::endl;
        return 1;
    }

    std::cout << "Decrypted data: " << decrypted_data << std::endl;

    return 0;
}