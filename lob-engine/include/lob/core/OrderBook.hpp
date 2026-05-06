#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <emmintrin.h>
#include "lob/core/Order.hpp"
#include "lob/core/ExecutionReport.hpp"
#include "lob/core/PriceLevel.hpp"
#include "lob/memory/slab_allocator.hpp"
#include "lob/concurrency/SPSCQueue.hpp"

namespace lob::core {
    static constexpr uint32_t NULL_IDX_32 = 0xFFFFFFFF;
    // OrderBook is two flattened vectors (bids, asks) of PriceLevels, of n length
    // where n is a static variable determining the number of ticks to support
    // e.g. to support $0 - $100.05, two vectors of size 10,005 are needed
    class OrderBook{

    private:
        
        std::vector<PriceLevel> bids;
        std::vector<PriceLevel> asks;

        uint64_t best_bid = 0;
        uint64_t best_ask = 0xFFFFFFFFFFFFFFFF;
        size_t max_ticks;
        lob::concurrency::SPSCQueue<lob::core::ExecutionReport, 1024>& egress_queue;

        public:

            explicit OrderBook(size_t max_price_ticks, lob::concurrency::SPSCQueue<lob::core::ExecutionReport, 1024>& egress_queue) 
            : max_ticks(max_price_ticks), egress_queue(egress_queue) {

                bids.resize(max_price_ticks);
                asks.resize(max_price_ticks);

            }

            // helpful getters for when we match on new orders
            // to prevent time waste searching for bids/asks below 
            // certain threshhold
            inline uint64_t get_best_bid() const {return best_bid;}
            inline uint64_t get_best_ask() const {return best_ask;}
        
        public:

            inline void add_order(uint32_t order_idx, lob::memory::OrderPool& pool) {

                Order& order = pool.get(order_idx);
                if (order.is_buy) {
                    process_buy(order_idx, order, pool);
                }
                else {
                    process_sell(order_idx, order, pool);
                }

            }
        
        private: 

            // process buy processes orders of type buy, recieved from submit_order API
            // all orders follow similar instructions: 
            // match order to price level, execute the trade, and either exhaust or rest the order 
            inline void process_buy(uint32_t incoming_idx, Order& incoming, lob::memory::OrderPool& pool) {

                while (incoming.quantity > 0 && incoming.price >= best_ask) {

                    PriceLevel& ask_level = asks[best_ask];
                    // no asks at current price
                    if (ask_level.is_empty()) {
                        best_ask++;
                        if(best_ask >= max_ticks) {
                            break;
                        }
                        continue;
                    }

                    // there are asks so start traversing price level and matching
                    uint32_t resting_idx = ask_level.get_head();
                    while (resting_idx != NULL_IDX_32 && incoming.quantity > 0) {
                        
                        Order& resting = pool.get(resting_idx);
                        uint32_t filled_qty = std::min(resting.quantity, incoming.quantity);

                        resting.quantity -= filled_qty;
                        incoming.quantity -= filled_qty;
                        // push report to egress ring buffer
                        lob::core::ExecutionReport curr_report;
                        curr_report.buy_id = incoming.id;
                        curr_report.sell_id = resting.id;
                        // realistically, make this a double to avoid losing
                        // some data
                        curr_report.matched_price = resting.price / 100.0;
                        curr_report.matched_quantity = filled_qty;
                        // wait to be picked up by egress thread
                        while (!egress_queue.push(curr_report)) {
                            _mm_pause();
                        }
                        
                        uint32_t next_resting_idx = resting.next_order_idx;
                        if (resting.quantity == 0) {
                            // remove from price level
                            ask_level.remove_order(resting_idx, pool);
                            // return memory
                            pool.deallocate(resting_idx);
                        } 
                        resting_idx = next_resting_idx;
                    }

                    if (ask_level.is_empty()) {
                        best_ask++;
                        if(best_ask >= max_ticks) {
                            break;
                        }
                }
            }
                    
            // if order isn't exhausted rest in its price level
            if (incoming.quantity > 0) {
                bids[incoming.price].append_order(incoming_idx, pool);
                if (incoming.price > best_ask) {
                    best_bid = incoming.price;
                }
            }
            else {
                pool.deallocate(incoming_idx);
            }
            }

            inline void process_sell(uint32_t incoming_idx, Order& incoming, lob::memory::OrderPool& pool) {

                while (incoming.quantity > 0 && incoming.price <= best_bid) {

                    PriceLevel& bid_level = bids[best_bid];
                    // no bids at current price
                    if (bid_level.is_empty()) {
                        // end of book
                        if(best_bid == 0) {
                            break;
                        }
                        best_bid--;
                        continue;
                    }

                    // there are asks so start traversing price level and matching
                    uint32_t resting_idx = bid_level.get_head();
                    while (resting_idx != NULL_IDX_32 && incoming.quantity > 0) {
                        
                        Order& resting = pool.get(resting_idx);
                        uint32_t filled_qty = std::min(resting.quantity, incoming.quantity);

                        resting.quantity -= filled_qty;
                        incoming.quantity -= filled_qty;

                        // push report to egress ring buffer
                        lob::core::ExecutionReport curr_report;
                        curr_report.sell_id = incoming.id;
                        curr_report.buy_id = resting.id;
                        // log the resting price as in its cent form
                        // then perform arithmetic at the egress level
                        curr_report.matched_price = resting.price;
                        curr_report.matched_quantity = filled_qty;
                        // wait to be popped by egress thread
                        while (!egress_queue.push(curr_report)) {
                            _mm_pause();
                        }
                        
                        int32_t next_resting_idx = resting.next_order_idx;

                        if (resting.quantity == 0) {
                            // remove from price level
                            bid_level.remove_order(resting_idx, pool);
                            // return memory
                            pool.deallocate(resting_idx);
                        } 

                        resting_idx = next_resting_idx;
                    }

                    if (bid_level.is_empty()) {
                        // end of book
                        if(best_bid == 0) {
                            break;
                        }
                        best_bid--;
                        continue;
                    }
                }
                    
                // if order isn't exhausted rest in its price level
                if (incoming.quantity > 0) {
                    asks[incoming.price].append_order(incoming_idx, pool);
                    if (incoming.price < best_ask) {
                        best_ask = incoming.price;
                    }
                }
                else {
                    pool.deallocate(incoming_idx);
            }
        }
    };
}