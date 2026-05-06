#pragma once
#include <cstdint>
#include "lob/memory/slab_allocator.hpp"

namespace lob::core {
struct PriceLevel {
    static constexpr uint32_t NULL_IDX_32 = 0xFFFFFFFF;
    uint64_t price = 0;
    uint32_t head_idx = NULL_IDX_32;
    uint32_t tail_idx = NULL_IDX_32;
    
    inline uint32_t get_head() {return head_idx;}

    inline bool is_empty() {return head_idx == NULL_IDX_32;}
    // O(1) append
    inline void append_order(uint32_t order_idx, lob::memory::OrderPool& pool) {
        Order& new_order = pool.get(order_idx);
        new_order.next_order_idx = NULL_IDX_32;
        new_order.prev_order_idx = tail_idx;
        if (is_empty()) [[unlikely]] {
            // no node exists, so first node is both head
            // and tail
            head_idx = order_idx;
            tail_idx = order_idx;
        }
        else {
            // append to end of linked list
            Order& old_tail = pool.get(tail_idx);
            old_tail.next_order_idx = order_idx;
            tail_idx = order_idx;
        }
    }
    // O(1) removal
    inline void remove_order (uint32_t order_idx, lob::memory::OrderPool& pool) {

        Order& order_to_remove = pool.get(order_idx);
        // if node is the start of list remove and ensure next node is head
        if (order_to_remove.prev_order_idx == NULL_IDX_32) {
            head_idx = order_to_remove.next_order_idx;
        }
        // otherwise, just remove
        else {
            pool.get(order_to_remove.prev_order_idx).next_order_idx = order_to_remove.next_order_idx;
        }
        // if node is the end of list, remove and ensure previous node is tail
        if (order_to_remove.next_order_idx == NULL_IDX_32) {
            tail_idx = order_to_remove.prev_order_idx;
        }
        // otherwise, just remove
        else {
            pool.get(order_to_remove.next_order_idx).prev_order_idx = order_to_remove.prev_order_idx;
        }

        order_to_remove.prev_order_idx = NULL_IDX_32;
        order_to_remove.next_order_idx = NULL_IDX_32;

    }

};

}