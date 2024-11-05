#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>

const int MIN_DEGREE = 3;

class BTreeNode : public std::enable_shared_from_this<BTreeNode> {
public:
    bool leaf;
    int numKeys;
    std::vector<int> keys;
    std::vector<std::shared_ptr<BTreeNode>> children;

    BTreeNode(bool _leaf) {
        leaf = _leaf;
        numKeys = 0;
        keys.resize(2 * MIN_DEGREE - 1);
        children.resize(2 * MIN_DEGREE);
    }

    void insertNonFull(int k);
    void splitChild(int i, std::shared_ptr<BTreeNode> y);
    void traverse();
    std::shared_ptr<BTreeNode> search(int k);
};

class BTree {
public:
    std::shared_ptr<BTreeNode> root;

    BTree() {
        root = nullptr;
    }

    void traverse() {
        if (root != nullptr)
            root->traverse();
    }

    std::shared_ptr<BTreeNode> search(int k) {
        return (root == nullptr) ? nullptr : root->search(k);
    }

    void insert(int k);
};

void BTreeNode::traverse() {
    int i;
    for (i = 0; i < numKeys; i++) {
        if (!leaf) {
            children[i]->traverse();
        }
        std::cout << " " << keys[i];
    }

    if (!leaf) {
        children[i]->traverse();
    }
}

std::shared_ptr<BTreeNode> BTreeNode::search(int k) {
    int i = 0;
    while (i < numKeys && k > keys[i])
        i++;

    if (i < numKeys && keys[i] == k)
        return shared_from_this();

    if (leaf)
        return nullptr;

    return children[i]->search(k);
}

void BTree::insert(int k) {
    if (root == nullptr) {
        root = std::make_shared<BTreeNode>(true);
        root->keys[0] = k;
        root->numKeys = 1;
    } else {
        if (root->numKeys == 2 * MIN_DEGREE - 1) {
            std::shared_ptr<BTreeNode> s = std::make_shared<BTreeNode>(false);
            s->children[0] = root;
            s->splitChild(0, root);

            int i = 0;
            if (s->keys[0] < k)
                i++;
            s->children[i]->insertNonFull(k);

            root = s;
        } else {
            root->insertNonFull(k);
        }
    }
}

void BTreeNode::insertNonFull(int k) {
    int i = numKeys - 1;

    if (leaf) {
        while (i >= 0 && keys[i] > k) {
            keys[i + 1] = keys[i];
            i--;
        }

        keys[i + 1] = k;
        numKeys = numKeys + 1;
    } else {
        while (i >= 0 && keys[i] > k)
            i--;

        if (children[i + 1]->numKeys == 2 * MIN_DEGREE - 1) {
            splitChild(i + 1, children[i + 1]);

            if (keys[i + 1] < k)
                i++;
        }
        children[i + 1]->insertNonFull(k);
    }
}

void BTreeNode::splitChild(int i, std::shared_ptr<BTreeNode> y) {
    std::shared_ptr<BTreeNode> z = std::make_shared<BTreeNode>(y->leaf);
    z->numKeys = MIN_DEGREE - 1;

    for (int j = 0; j < MIN_DEGREE - 1; j++)
        z->keys[j] = y->keys[j + MIN_DEGREE];

    if (!y->leaf) {
        for (int j = 0; j < MIN_DEGREE; j++)
            z->children[j] = y->children[j + MIN_DEGREE];
    }

    y->numKeys = MIN_DEGREE - 1;

    for (int j = numKeys; j >= i + 1; j--)
        children[j + 1] = children[j];

    children[i + 1] = z;

    for (int j = numKeys - 1; j >= i; j--)
        keys[j + 1] = keys[j];

    keys[i] = y->keys[MIN_DEGREE - 1];
    numKeys = numKeys + 1;
}

int main() {
    BTree t;
    t.insert(10);
    t.insert(20);
    t.insert(5);
    t.insert(6);
    t.insert(12);
    t.insert(30);
    t.insert(7);
    t.insert(17);

    std::cout << "Traversal of the constructed B-tree is:\n";
    t.traverse();

    int key = 6;
    if (t.search(key) != nullptr)
        std::cout << "\nKey " << key << " is present in the tree.\n";
    else
        std::cout << "\nKey " << key << " is not present in the tree.\n";

    return 0;
}