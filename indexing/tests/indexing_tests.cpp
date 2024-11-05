#include <gtest/gtest.h>
#include "../b_tree/b_tree_index.h"
#include "../hash_index/hash_index.h"
#include "../trie_index/trie_index.h"

// Test fixture for B-tree index
class BTreeIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize B-tree with default settings
        b_tree = new BTreeIndex<int, std::string>();
        for (int i = 0; i < 100; ++i) {
            b_tree->insert(i, "value" + std::to_string(i));
        }
    }

    void TearDown() override {
        delete b_tree;
    }

    BTreeIndex<int, std::string>* b_tree;
};

// B-tree insertion test
TEST_F(BTreeIndexTest, InsertionTest) {
    EXPECT_NO_THROW(b_tree->insert(101, "value101"));
    EXPECT_EQ(b_tree->search(101), "value101");
}

// B-tree search test
TEST_F(BTreeIndexTest, SearchTest) {
    EXPECT_EQ(b_tree->search(50), "value50");
    EXPECT_EQ(b_tree->search(99), "value99");
}

// B-tree deletion test
TEST_F(BTreeIndexTest, DeletionTest) {
    EXPECT_NO_THROW(b_tree->remove(50));
    EXPECT_THROW(b_tree->search(50), std::out_of_range);
}

// B-tree range query test
TEST_F(BTreeIndexTest, RangeQueryTest) {
    auto results = b_tree->range_query(20, 30);
    EXPECT_EQ(results.size(), 11);
    EXPECT_EQ(results[0], "value20");
    EXPECT_EQ(results[10], "value30");
}

// Test fixture for Hash Index
class HashIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize hash index
        hash_index = new HashIndex<int, std::string>();
        for (int i = 0; i < 100; ++i) {
            hash_index->insert(i, "hash_value" + std::to_string(i));
        }
    }

    void TearDown() override {
        delete hash_index;
    }

    HashIndex<int, std::string>* hash_index;
};

// Hash index insertion test
TEST_F(HashIndexTest, InsertionTest) {
    EXPECT_NO_THROW(hash_index->insert(101, "hash_value101"));
    EXPECT_EQ(hash_index->search(101), "hash_value101");
}

// Hash index search test
TEST_F(HashIndexTest, SearchTest) {
    EXPECT_EQ(hash_index->search(50), "hash_value50");
    EXPECT_EQ(hash_index->search(99), "hash_value99");
}

// Hash index deletion test
TEST_F(HashIndexTest, DeletionTest) {
    EXPECT_NO_THROW(hash_index->remove(50));
    EXPECT_THROW(hash_index->search(50), std::out_of_range);
}

// Hash index collision handling test
TEST_F(HashIndexTest, CollisionHandlingTest) {
    hash_index->insert(1, "colliding_value");
    EXPECT_EQ(hash_index->search(1), "colliding_value");
}

// Test fixture for Trie Index
class TrieIndexTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize trie index
        trie_index = new TrieIndex();
        trie_index->insert("apple");
        trie_index->insert("app");
        trie_index->insert("apricot");
        trie_index->insert("banana");
    }

    void TearDown() override {
        delete trie_index;
    }

    TrieIndex* trie_index;
};

// Trie index insertion test
TEST_F(TrieIndexTest, InsertionTest) {
    EXPECT_NO_THROW(trie_index->insert("pear"));
    EXPECT_TRUE(trie_index->search("pear"));
}

// Trie index search test
TEST_F(TrieIndexTest, SearchTest) {
    EXPECT_TRUE(trie_index->search("apple"));
    EXPECT_TRUE(trie_index->search("app"));
    EXPECT_FALSE(trie_index->search("apples"));
}

// Trie index deletion test
TEST_F(TrieIndexTest, DeletionTest) {
    EXPECT_NO_THROW(trie_index->remove("banana"));
    EXPECT_FALSE(trie_index->search("banana"));
}

// Trie prefix search test
TEST_F(TrieIndexTest, PrefixSearchTest) {
    auto results = trie_index->prefix_search("app");
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0], "apple");
    EXPECT_EQ(results[1], "app");
}

// Main function for running all tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}