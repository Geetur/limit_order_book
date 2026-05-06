#pragma once

#include <cstdint>

namespace lob::core {
struct OrderRequest{

    uint64_t order_id; // 8 byte
    uint64_t price; // 8 byte
    uint32_t quantity; // 4 byte
    bool is_buy; // 1 byte

};
};