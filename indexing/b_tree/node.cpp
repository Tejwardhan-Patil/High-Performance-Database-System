#include <iostream>
#include <vector>
#include <memory>

const int MIN_DEGREE = 3;  // Minimum degree (defines the range for number of keys)

class BTreeNode {
public:
    BTreeNode(bool leaf);
    void traverse();  // Traverses all nodes in the B-tree
    BTreeNode* search(int key);  // Searches for a key in the subtree rooted at this node

    // Insert a new key into the non-full node
    void insertNonFull(int key);
    
    // Split the child 'i' of this node
    void splitChild(int i, std::shared_ptr<BTreeNode> y);

    int findKey(int key);

    // Remove the key from the subtree rooted at this node
    void remove(int key);

    void removeFromLeaf(int idx);
    void removeFromNonLeaf(int idx);

    int getPred(int idx);
    int getSucc(int idx);

    void fill(int idx);
    void borrowFromPrev(int idx);
    void borrowFromNext(int idx);
    void merge(int idx);

    friend class BTree;

private:
    int* keys;  // An array of keys
    int t;      // Minimum degree (defines the range for number of keys)
    std::shared_ptr<BTreeNode>* children;  // An array of child pointers
    int n;      // Current number of keys
    bool leaf;  // Is true when the node is a leaf, otherwise false
};

BTreeNode::BTreeNode(bool leaf) {
    this->leaf = leaf;
    this->t = MIN_DEGREE;

    keys = new int[2 * t - 1];
    children = new std::shared_ptr<BTreeNode>[2 * t];
    n = 0;
}

int BTreeNode::findKey(int key) {
    int idx = 0;
    while (idx < n && keys[idx] < key)
        ++idx;
    return idx;
}

void BTreeNode::remove(int key) {
    int idx = findKey(key);

    if (idx < n && keys[idx] == key) {
        if (leaf) {
            removeFromLeaf(idx);
        } else {
            removeFromNonLeaf(idx);
        }
    } else {
        if (leaf) {
            std::cout << "The key " << key << " is not present in the tree\n";
            return;
        }

        bool flag = ((idx == n) ? true : false);

        if (children[idx]->n < t) {
            fill(idx);
        }

        if (flag && idx > n)
            children[idx - 1]->remove(key);
        else
            children[idx]->remove(key);
    }
}

void BTreeNode::removeFromLeaf(int idx) {
    for (int i = idx + 1; i < n; ++i)
        keys[i - 1] = keys[i];
    n--;
}

void BTreeNode::removeFromNonLeaf(int idx) {
    int key = keys[idx];

    if (children[idx]->n >= t) {
        int pred = getPred(idx);
        keys[idx] = pred;
        children[idx]->remove(pred);
    } else if (children[idx + 1]->n >= t) {
        int succ = getSucc(idx);
        keys[idx] = succ;
        children[idx + 1]->remove(succ);
    } else {
        merge(idx);
        children[idx]->remove(key);
    }
}

int BTreeNode::getPred(int idx) {
    BTreeNode* cur = children[idx].get();
    while (!cur->leaf)
        cur = cur->children[cur->n - 1].get();

    return cur->keys[cur->n - 1];
}

int BTreeNode::getSucc(int idx) {
    BTreeNode* cur = children[idx + 1].get();
    while (!cur->leaf)
        cur = cur->children[0].get();

    return cur->keys[0];
}

void BTreeNode::fill(int idx) {
    if (idx != 0 && children[idx - 1]->n >= t)
        borrowFromPrev(idx);
    else if (idx != n && children[idx + 1]->n >= t)
        borrowFromNext(idx);
    else {
        if (idx != n)
            merge(idx);
        else
            merge(idx - 1);
    }
}

void BTreeNode::borrowFromPrev(int idx) {
    BTreeNode* child = children[idx].get();
    BTreeNode* sibling = children[idx - 1].get();

    for (int i = child->n - 1; i >= 0; --i)
        child->keys[i + 1] = child->keys[i];

    if (!child->leaf) {
        for (int i = child->n; i >= 0; --i)
            child->children[i + 1] = child->children[i];
    }

    child->keys[0] = keys[idx - 1];
    if (!leaf)
        child->children[0] = sibling->children[sibling->n];

    keys[idx - 1] = sibling->keys[sibling->n - 1];
    child->n += 1;
    sibling->n -= 1;
}

void BTreeNode::borrowFromNext(int idx) {
    BTreeNode* child = children[idx].get();
    BTreeNode* sibling = children[idx + 1].get();

    child->keys[child->n] = keys[idx];

    if (!child->leaf)
        child->children[child->n + 1] = sibling->children[0];

    keys[idx] = sibling->keys[0];

    for (int i = 1; i < sibling->n; ++i)
        sibling->keys[i - 1] = sibling->keys[i];

    if (!sibling->leaf) {
        for (int i = 1; i <= sibling->n; ++i)
            sibling->children[i - 1] = sibling->children[i];
    }

    child->n += 1;
    sibling->n -= 1;
}

void BTreeNode::merge(int idx) {
    BTreeNode* child = children[idx].get();
    BTreeNode* sibling = children[idx + 1].get();

    child->keys[t - 1] = keys[idx];

    for (int i = 0; i < sibling->n; ++i)
        child->keys[i + t] = sibling->keys[i];

    if (!child->leaf) {
        for (int i = 0; i <= sibling->n; ++i)
            child->children[i + t] = sibling->children[i];
    }

    for (int i = idx + 1; i < n; ++i)
        keys[i - 1] = keys[i];

    for (int i = idx + 2; i <= n; ++i)
        children[i - 1] = children[i];

    child->n += sibling->n + 1;
    n--;
}

BTreeNode* BTreeNode::search(int key) {
    int i = 0;
    while (i < n && key > keys[i])
        i++;

    if (keys[i] == key)
        return this;

    if (leaf)
        return nullptr;

    return children[i]->search(key);
}

void BTreeNode::insertNonFull(int key) {
    int i = n - 1;

    if (leaf) {
        while (i >= 0 && keys[i] > key) {
            keys[i + 1] = keys[i];
            i--;
        }
        keys[i + 1] = key;
        n = n + 1;
    } else {
        while (i >= 0 && keys[i] > key)
            i--;

        if (children[i + 1]->n == 2 * t - 1) {
            splitChild(i + 1, children[i + 1]);

            if (keys[i + 1] < key)
                i++;
        }
        children[i + 1]->insertNonFull(key);
    }
}

void BTreeNode::splitChild(int i, std::shared_ptr<BTreeNode> y) {
    auto z = std::make_shared<BTreeNode>(y->leaf);
    z->n = t - 1;

    for (int j = 0; j < t - 1; j++)
        z->keys[j] = y->keys[j + t];

    if (!y->leaf) {
        for (int j = 0; j < t; j++)
            z->children[j] = y->children[j + t];
    }

    y->n = t - 1;

    for (int j = n; j >= i + 1; j--)
        children[j + 1] = children[j];

    children[i + 1] = z;

    for (int j = n - 1; j >= i; j--)
        keys[j + 1] = keys[j];

    keys[i] = y->keys[t - 1];
    n = n + 1;
}