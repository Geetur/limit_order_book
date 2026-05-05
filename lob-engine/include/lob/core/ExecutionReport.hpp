#pragma once

#include <cstdint>

namespace lob::core {
struct  alignas(32) ExecutionReport {
    // 28 total bytes, 4 padding
    uint64_t buy_id; // 8 bytes
    uint64_t sell_id; // 8 bytes
    uint64_t matched_price; // 8 bytes
    uint32_t matched_quantity; // 4 bytes
};
}
