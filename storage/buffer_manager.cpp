#include <iostream>
#include <unordered_map>
#include <list>
#include <vector>
#include <mutex>
#include <memory>
#include <condition_variable>

class Page {
public:
    int page_id;
    bool dirty;
    std::vector<char> data;

    Page(int id) : page_id(id), dirty(false), data(4096) {}
};

class DiskStorage {
public:
    void read_page(int page_id, std::vector<char>& buffer) {
        // Simulate reading from disk
        std::cout << "Reading page " << page_id << " from disk.\n";
    }

    void write_page(int page_id, const std::vector<char>& buffer) {
        // Simulate writing to disk
        std::cout << "Writing page " << page_id << " to disk.\n";
    }
};

class BufferManager {
private:
    int capacity;
    DiskStorage* disk_storage;

    // Eviction policy
    std::list<int> lru_list;
    std::unordered_map<int, std::shared_ptr<Page>> page_table;

    // Mutex for thread safety
    std::mutex mtx;
    std::condition_variable cv;

    void evict_page() {
        while (page_table.size() >= capacity) {
            int evict_id = lru_list.back();
            lru_list.pop_back();
            
            auto page = page_table[evict_id];
            if (page->dirty) {
                disk_storage->write_page(page->page_id, page->data);
            }
            page_table.erase(evict_id);
            std::cout << "Evicted page " << evict_id << "\n";
        }
    }

public:
    BufferManager(int cap, DiskStorage* storage)
        : capacity(cap), disk_storage(storage) {}

    std::shared_ptr<Page> fetch_page(int page_id) {
        std::unique_lock<std::mutex> lock(mtx);

        // If the page is in the buffer pool, return it
        if (page_table.find(page_id) != page_table.end()) {
            lru_list.remove(page_id);
            lru_list.push_front(page_id);
            return page_table[page_id];
        }

        // If not, evict if necessary and load the page
        evict_page();

        auto new_page = std::make_shared<Page>(page_id);
        disk_storage->read_page(page_id, new_page->data);
        page_table[page_id] = new_page;
        lru_list.push_front(page_id);

        return new_page;
    }

    void mark_dirty(int page_id) {
        std::unique_lock<std::mutex> lock(mtx);
        if (page_table.find(page_id) != page_table.end()) {
            page_table[page_id]->dirty = true;
        }
    }

    void flush_page(int page_id) {
        std::unique_lock<std::mutex> lock(mtx);
        if (page_table.find(page_id) != page_table.end() && page_table[page_id]->dirty) {
            auto page = page_table[page_id];
            disk_storage->write_page(page_id, page->data);
            page->dirty = false;
        }
    }

    void flush_all_pages() {
        std::unique_lock<std::mutex> lock(mtx);
        for (auto& entry : page_table) {
            if (entry.second->dirty) {
                disk_storage->write_page(entry.first, entry.second->data);
                entry.second->dirty = false;
            }
        }
    }
};

// Test class for buffer manager
void test_buffer_manager() {
    DiskStorage disk;
    BufferManager buffer_mgr(3, &disk);

    auto page1 = buffer_mgr.fetch_page(1);
    page1->data[0] = 'A';
    buffer_mgr.mark_dirty(1);

    auto page2 = buffer_mgr.fetch_page(2);
    page2->data[0] = 'B';
    buffer_mgr.mark_dirty(2);

    auto page3 = buffer_mgr.fetch_page(3);
    page3->data[0] = 'C';
    buffer_mgr.mark_dirty(3);

    buffer_mgr.fetch_page(4);  

    buffer_mgr.flush_all_pages();
}

int main() {
    test_buffer_manager();
    return 0;
}