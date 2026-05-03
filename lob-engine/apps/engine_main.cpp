#include <iostream>
#include <lob/memory/slab_allocator.hpp>
#include <lob/core/OrderBook.hpp>


 void test_OrderPool_build () {
        
    std::cout << "building OrderPool with 1,000,000 capacity...\n";

    lob::memory::OrderPool pool(1000000);

    uint32_t new_idx = pool.allocate();

    lob::core::Order& my_order = pool.get(new_idx);

    my_order.id = 1;
    my_order.is_buy = true;
    my_order.price = 10500;
    my_order.quantity = 500;

    std::cout << "Successfully allocated Order ID: " << my_order.id << "\n";
    std::cout << "Returning memory to pool...\n";

    pool.deallocate(new_idx);
};

void test_sell_and_buy() {
    lob::memory::OrderPool pool(1000000);
    // Create an OrderBook supporting up to $200.00 (represented in cents as 20000)
    lob::core::OrderBook book(20000); 

    std::cout << "--- 1. Submitting Resting Sell Order ---\n";
    uint32_t sell_idx = pool.allocate();
    lob::core::Order& sell_order = pool.get(sell_idx);
    sell_order.id = 101;
    sell_order.price = 10050; // Willing to sell at $100.50
    sell_order.quantity = 100;
    sell_order.is_buy = false;
    
    book.add_order(sell_idx, pool);
    std::cout << "Sell Order resting in book.\n\n";

    std::cout << "--- 2. Submitting Aggressive Buy Order ---\n";
    uint32_t buy_idx = pool.allocate();
    lob::core::Order& buy_order = pool.get(buy_idx);
    buy_order.id = 102;
    buy_order.price = 10050; // Willing to buy at $100.50
    buy_order.quantity = 75; // Only wants 75 shares
    buy_order.is_buy = true;

    book.add_order(buy_idx, pool);
    
    std::cout << "\n--- End of Simulation ---\n";
}


int main() {
    // test_OrderPool_build();
    test_sell_and_buy();
    return 0;

}