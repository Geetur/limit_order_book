#pragma once

#include <vector>
#include <cstdint>

namespace lob::core {
struct alignas(32) Order {

    uint64_t id; // 8 bytes
    uint64_t price; // 8 bytes
    uint32_t quantity; // 4 bytes
    bool is_buy; // 1 byte
    
    uint32_t next_order_idx; // 4 bytes
    uint32_t prev_order_idx; // 4 bytes

    // add three bytes padding to easily fit
    // in 64-byte l1 cache line
};
}
