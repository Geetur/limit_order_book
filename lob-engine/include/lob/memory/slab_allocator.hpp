#pragma once
#include <vector>
#include <cstdint>
#include "lob/core/Order.hpp"
#include <stdexcept>

namespace lob::memory {

class OrderPool {

private:
    std::vector<core::Order> pool;
    std::vector<uint32_t> free_list;

public:

   explicit OrderPool(size_t capacity) {
        pool.resize(capacity);
        free_list.reserve(capacity);

        for (uint32_t i = capacity; i > 0; i--) {
            free_list.push_back(i - 1);
        }
    }

    inline uint32_t allocate() {
        // unlikely tag for the compiler to optimize
        // hot patg
        if (free_list.empty()) [[unlikely]] {
            throw std::runtime_error("OrderPool Exhausted");
        }
        uint32_t idx = free_list.back();
        free_list.pop_back();
        return idx;
    }

    inline void deallocate(uint32_t idx) {
        free_list.push_back(idx);
    }

    inline core::Order& get(uint32_t idx) {
        return pool[idx];
    }
};
}

