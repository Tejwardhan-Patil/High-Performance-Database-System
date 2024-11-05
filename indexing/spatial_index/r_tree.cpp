#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>

const int MAX_CHILDREN = 4;
const int MIN_CHILDREN = 2;

struct Rect {
    float xmin, ymin, xmax, ymax;
    
    bool contains(const Rect& other) const {
        return xmin <= other.xmin && xmax >= other.xmax &&
               ymin <= other.ymin && ymax >= other.ymax;
    }

    bool intersects(const Rect& other) const {
        return !(xmin > other.xmax || xmax < other.xmin || 
                 ymin > other.ymax || ymax < other.ymin);
    }

    float area() const {
        return (xmax - xmin) * (ymax - ymin);
    }

    float enlargement(const Rect& other) const {
        Rect enlarged;
        enlarged.xmin = std::min(xmin, other.xmin);
        enlarged.ymin = std::min(ymin, other.ymin);
        enlarged.xmax = std::max(xmax, other.xmax);
        enlarged.ymax = std::max(ymax, other.ymax);
        return enlarged.area() - area();
    }
};

struct Node {
    Rect bounding_box;
    bool leaf;
    std::vector<Node*> children;
    Node* parent = nullptr;

    Node(bool is_leaf) : leaf(is_leaf) {}

    void updateBoundingBox() {
        if (children.empty()) return;
        bounding_box = children[0]->bounding_box;
        for (auto& child : children) {
            bounding_box.xmin = std::min(bounding_box.xmin, child->bounding_box.xmin);
            bounding_box.ymin = std::min(bounding_box.ymin, child->bounding_box.ymin);
            bounding_box.xmax = std::max(bounding_box.xmax, child->bounding_box.xmax);
            bounding_box.ymax = std::max(bounding_box.ymax, child->bounding_box.ymax);
        }
    }

    bool isFull() const {
        return children.size() >= MAX_CHILDREN;
    }

    bool isUnderfilled() const {
        return children.size() < MIN_CHILDREN;
    }
};

class RTree {
private:
    Node* root;

public:
    RTree() {
        root = new Node(true);
    }

    ~RTree() {
        clear(root);
    }

    void insert(const Rect& rect) {
        Node* leaf = chooseLeaf(root, rect);
        Node* new_node = new Node(true);
        new_node->bounding_box = rect;
        leaf->children.push_back(new_node);
        new_node->parent = leaf;
        adjustTree(leaf);
    }

    std::vector<Rect> search(const Rect& rect) {
        std::vector<Rect> result;
        searchHelper(root, rect, result);
        return result;
    }

private:
    Node* chooseLeaf(Node* node, const Rect& rect) {
        if (node->leaf) return node;

        Node* best_child = nullptr;
        float min_enlargement = std::numeric_limits<float>::max();

        for (auto& child : node->children) {
            float enlargement = child->bounding_box.enlargement(rect);
            if (enlargement < min_enlargement) {
                min_enlargement = enlargement;
                best_child = child;
            }
        }
        return chooseLeaf(best_child, rect);
    }

    void adjustTree(Node* node) {
        if (node == nullptr) return;

        node->updateBoundingBox();

        if (node->isFull()) {
            Node* new_node = splitNode(node);
            if (node == root) {
                root = new Node(false);
                root->children.push_back(node);
                root->children.push_back(new_node);
                node->parent = root;
                new_node->parent = root;
            } else {
                node->parent->children.push_back(new_node);
                new_node->parent = node->parent;
                adjustTree(node->parent);
            }
        } else {
            adjustTree(node->parent);
        }
    }

    Node* splitNode(Node* node) {
        std::vector<Node*> children = node->children;
        std::sort(children.begin(), children.end(), [](Node* a, Node* b) {
            return a->bounding_box.xmin < b->bounding_box.xmin;
        });

        Node* new_node = new Node(node->leaf);
        node->children.clear();
        int mid = children.size() / 2;

        for (int i = 0; i < mid; ++i) {
            node->children.push_back(children[i]);
        }

        for (int i = mid; i < children.size(); ++i) {
            new_node->children.push_back(children[i]);
        }

        node->updateBoundingBox();
        new_node->updateBoundingBox();

        return new_node;
    }

    void searchHelper(Node* node, const Rect& rect, std::vector<Rect>& result) {
        if (!node->bounding_box.intersects(rect)) return;

        if (node->leaf) {
            result.push_back(node->bounding_box);
        } else {
            for (auto& child : node->children) {
                searchHelper(child, rect, result);
            }
        }
    }

    void clear(Node* node) {
        if (!node->leaf) {
            for (auto& child : node->children) {
                clear(child);
            }
        }
        delete node;
    }
};

int main() {
    RTree rtree;
    Rect r1 = {0, 0, 10, 10};
    Rect r2 = {5, 5, 15, 15};
    Rect r3 = {20, 20, 30, 30};

    rtree.insert(r1);
    rtree.insert(r2);
    rtree.insert(r3);

    Rect search_area = {0, 0, 15, 15};
    auto results = rtree.search(search_area);

    std::cout << "Search Results:\n";
    for (auto& rect : results) {
        std::cout << "Rect(" << rect.xmin << ", " << rect.ymin << ", " 
                  << rect.xmax << ", " << rect.ymax << ")\n";
    }

    return 0;
}