#include "../core/types.hpp"

class OrderPool {

private:
    std::vector<Order> pool;
    std::vector<uint32_t> free_list;

public:
    OrderPool(size_t capacity) {
        pool.resize(capacity);
        free_list.reserve(capacity);

        for (uint32_t i = capacity - 1; i >= 0; i--) {
            free_list.push_back(i);
        }
    }

    uint32_t allocate() {
        if (free_list.empty()) {return -1}
        uint32_t idx = free_list.back();
        free_list.pop_back();
        return idx;
    }

    void deallocate(uint32_t idx) {
        free_list.push_back(idx);
    }
          
&Order get(idx) {return pool[idx]};
};


