#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>

class TrieNode {
public:
    std::unordered_map<char, TrieNode*> children;
    bool is_end_of_word;

    TrieNode() : is_end_of_word(false) {}
};

class Trie {
private:
    TrieNode* root;

    void delete_trie(TrieNode* node) {
        for (auto& pair : node->children) {
            delete_trie(pair.second);
        }
        delete node;
    }

    void collect_words(TrieNode* node, std::string& current_prefix, std::vector<std::string>& words) {
        if (node->is_end_of_word) {
            words.push_back(current_prefix);
        }
        for (auto& pair : node->children) {
            current_prefix.push_back(pair.first);
            collect_words(pair.second, current_prefix, words);
            current_prefix.pop_back();
        }
    }

    TrieNode* search_prefix(const std::string& prefix) const {
        TrieNode* node = root;
        for (char ch : prefix) {
            if (node->children.find(ch) == node->children.end()) {
                return nullptr;
            }
            node = node->children[ch];
        }
        return node;
    }

public:
    Trie() {
        root = new TrieNode();
    }

    ~Trie() {
        delete_trie(root);
    }

    void insert(const std::string& word) {
        TrieNode* node = root;
        for (char ch : word) {
            if (node->children.find(ch) == node->children.end()) {
                node->children[ch] = new TrieNode();
            }
            node = node->children[ch];
        }
        node->is_end_of_word = true;
    }

    bool search(const std::string& word) const {
        TrieNode* node = search_prefix(word);
        return node != nullptr && node->is_end_of_word;
    }

    bool starts_with(const std::string& prefix) const {
        return search_prefix(prefix) != nullptr;
    }

    std::vector<std::string> get_words_with_prefix(const std::string& prefix) {
        std::vector<std::string> result;
        TrieNode* node = search_prefix(prefix);
        if (node == nullptr) return result;

        std::string current_prefix = prefix;
        collect_words(node, current_prefix, result);
        return result;
    }

    bool remove(const std::string& word) {
        return remove_helper(root, word, 0);
    }

private:
    bool remove_helper(TrieNode* node, const std::string& word, int depth) {
        if (depth == word.length()) {
            if (!node->is_end_of_word) return false;
            node->is_end_of_word = false;
            return node->children.empty();
        }

        char ch = word[depth];
        if (node->children.find(ch) == node->children.end()) return false;

        TrieNode* child = node->children[ch];
        bool should_delete_child = remove_helper(child, word, depth + 1);

        if (should_delete_child) {
            node->children.erase(ch);
            delete child;
            return node->children.empty() && !node->is_end_of_word;
        }

        return false;
    }
};

// Usage of the Trie for indexing
int main() {
    Trie trie_index;

    // Inserting words
    trie_index.insert("apple");
    trie_index.insert("app");
    trie_index.insert("application");
    trie_index.insert("banana");
    trie_index.insert("band");
    trie_index.insert("bandwidth");

    // Searching for a word
    std::cout << "Search 'app': " << trie_index.search("app") << std::endl;
    std::cout << "Search 'apple': " << trie_index.search("apple") << std::endl;
    std::cout << "Search 'ban': " << trie_index.search("ban") << std::endl;

    // Checking prefix
    std::cout << "Prefix 'app': " << trie_index.starts_with("app") << std::endl;
    std::cout << "Prefix 'ban': " << trie_index.starts_with("ban") << std::endl;

    // Getting all words with prefix
    std::vector<std::string> words_with_prefix = trie_index.get_words_with_prefix("app");
    std::cout << "Words with prefix 'app': ";
    for (const auto& word : words_with_prefix) {
        std::cout << word << " ";
    }
    std::cout << std::endl;

    // Removing a word
    std::cout << "Remove 'apple': " << trie_index.remove("apple") << std::endl;
    std::cout << "Search 'apple' after removal: " << trie_index.search("apple") << std::endl;

    return 0;
}