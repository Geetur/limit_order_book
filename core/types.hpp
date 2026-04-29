#include <vector>
#include <cstdint>

struct Order {

    uint64_t id;
    uint64_t price;
    uint32_t quantity;
    bool is_buy;
    
    uint32_t next_order_idx;
    uint32_t prev_order_idx;
};
