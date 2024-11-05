#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <set>

using namespace std;

// Utility function to convert a string to lowercase
string to_lowercase(const string &s) {
    string result = s;
    transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return tolower(c); });
    return result;
}

// Tokenizes the content into words (split by spaces and punctuation)
vector<string> tokenize(const string &content) {
    vector<string> tokens;
    string token;
    stringstream ss(content);

    while (ss >> token) {
        token.erase(remove_if(token.begin(), token.end(), ::ispunct), token.end());
        tokens.push_back(to_lowercase(token));
    }
    return tokens;
}

// Inverted index class
class InvertedIndex {
private:
    // Dictionary to map words to document IDs where they occur
    unordered_map<string, set<int>> index;
    // Map of document IDs to filenames
    unordered_map<int, string> docIdToFile;
    int currentDocId;

public:
    InvertedIndex() : currentDocId(0) {}

    // Function to index a document given its content and filename
    void addDocument(const string &filename, const string &content) {
        vector<string> tokens = tokenize(content);
        docIdToFile[currentDocId] = filename;
        for (const string &token : tokens) {
            index[token].insert(currentDocId);
        }
        currentDocId++;
    }

    // Search for a word in the index and return the document IDs containing it
    vector<string> search(const string &query) {
        string lowercaseQuery = to_lowercase(query);
        if (index.find(lowercaseQuery) != index.end()) {
            set<int> docIds = index[lowercaseQuery];
            vector<string> results;
            for (int docId : docIds) {
                results.push_back(docIdToFile[docId]);
            }
            return results;
        } else {
            return {};  // No documents found
        }
    }

    // Display the entire inverted index (for debugging or inspection)
    void displayIndex() {
        for (const auto &pair : index) {
            cout << "Word: " << pair.first << " -> Documents: ";
            for (int docId : pair.second) {
                cout << docIdToFile[docId] << " ";
            }
            cout << endl;
        }
    }
};

// Utility function to read the content of a file
string read_file(const string &filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Could not open the file " << filename << endl;
        return "";
    }
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    InvertedIndex index;

    // Indexing sample documents
    string file1 = "document1.txt";
    string file2 = "document2.txt";

    string content1 = read_file(file1);
    string content2 = read_file(file2);

    if (!content1.empty()) {
        index.addDocument(file1, content1);
    }
    if (!content2.empty()) {
        index.addDocument(file2, content2);
    }

    // Display the inverted index (for debugging)
    cout << "Inverted Index:" << endl;
    index.displayIndex();

    // Perform a search
    string query;
    cout << "\nEnter a search query: ";
    cin >> query;
    
    vector<string> searchResults = index.search(query);
    if (!searchResults.empty()) {
        cout << "Documents containing the word '" << query << "':" << endl;
        for (const string &doc : searchResults) {
            cout << doc << endl;
        }
    } else {
        cout << "No documents contain the word '" << query << "'." << endl;
    }

    return 0;
}