#include <iostream>
#include <thread>
#include <emmintrin.h>
#include <lob/memory/slab_allocator.hpp>
#include <lob/core/OrderBook.hpp>
#include <lob/concurrency/SPSCQueue.hpp>


lob::concurrency::SPSCQueue<int, 1024> lock_free_queue;

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

void network_producer_thread() {
        std::cout << "Producer starting...\n";
        for (int i = 1; i <= 5; ++i) {
            // Spin until there is room in the queue
            while (!lock_free_queue.push(i)) {
                // CPU relaxes slightly while spinning
                _mm_pause();
            }
            std::cout << "Pushed: " << i << "\n";
        }
    }
    
void engine_consumer_thread() {
    std::cout << "Consumer starting...\n";
    int items_received = 0;
    int received_value = 0;

    while (items_received < 5) {
        // Spin infinitely checking for new data (Busy Polling)
        if (lock_free_queue.pop(received_value)) {
            std::cout << "          Popped: " << received_value << "\n";
            items_received++;
        }
    }
}



int main() {
    test_OrderPool_build();
    test_sell_and_buy();

    // test produces and consumer threads -----
    std::cout << "--- Lock-Free Queue Test ---\n";

    // Launch threads
    std::thread producer(network_producer_thread);
    std::thread consumer(engine_consumer_thread);

    // Wait for them to finish
    producer.join();
    consumer.join();

    std::cout << "--- Test Complete ---\n";

    return 0;

}