#pragma once

#include <cstdint>
#include <vector>
#include "../core/Order.hpp"
#include "../memory/slab_allocator.hpp"

namespace lob::core {

    // OrderBook is two flattened vectors (bids, asks) of PriceLevels, of n length
    // where n is a static variable determining the number of ticks to support
    // e.g. to support $0 - $100.05, two vectors of size 10,005 are needed
    class OrderBook{

    private:
        
        // PriceLevel is an intrusive linked list that features all orders at a given price point
        struct PriceLevel {

            static constexpr uint32_t NULL_IDX = 0xFFFFFFFF;
            uint64_t price = 0;
            uint32_t head_idx = NULL_IDX;
            uint32_t tail_idx = NULL_IDX;

            inline bool is_empty() {return head_idx == NULL_IDX;}
            // O(1) append
            inline void append_order(uint32_t order_idx, lob::memory::OrderPool& pool) {
                Order& new_order = pool.get(order_idx);
                new_order.next_order_idx = NULL_IDX;
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
                if (order_to_remove.prev_order_idx == NULL_IDX) [[unlikely]] {
                    head_idx = order_to_remove.next_order_idx;
                }
                // otherwise, just remove
                else {
                    pool.get(order_to_remove.prev_order_idx).next_order_idx = order_to_remove.next_order_idx;
                }
                // if node is the end of list, remove and ensure previous node is tail
                if (order_to_remove.next_order_idx == NULL_IDX) [[unlikely]] {
                    tail_idx = order_to_remove.prev_order_idx;
                }
                // otherwise, just remove
                else {
                    pool.get(order_to_remove.next_order_idx).prev_order_idx = order_to_remove.prev_order_idx;
                }

                order_to_remove.prev_order_idx = NULL_IDX;
                order_to_remove.next_order_idx = NULL_IDX;

            }

        };

        std::vector<PriceLevel> bids;
        std::vector<PriceLevel> asks;
        
        uint64_t best_bid = 0;
        uint64_t best_ask = 0xFFFFFFFFFFFFFFFF;

        public:

            explicit OrderBook(size_t max_price_ticks) {

                bids.resize(max_price_ticks);
                asks.resize(max_price_ticks);

            }

            inline void add_order(uint32_t order_idx, lob::memory::OrderPool& pool) {

                Order& order = pool.get(order_idx);

                if (order.is_buy) {

                    bids[order.price].append_order(order_idx, pool);
                    if (order.price > best_bid) {best_bid = order.price;}

                }
                else {
                    asks[order.price].append_order(order_idx, pool);
                    if (order.price < best_ask) {best_ask = order.price;}
                }


            }
            // helpful getters for when we match on new orders
            // to prevent time waste searching for bids/asks below 
            // certain threshhold
            inline uint64_t get_best_bid() const {return best_bid;}
            inline uint64_t get_best_ask() const {return best_ask;}
};
}