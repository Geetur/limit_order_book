#include <iostream>
#include <thread>
#include <emmintrin.h>
#include <lob/memory/slab_allocator.hpp>
#include <lob/core/OrderBook.hpp>
#include <lob/concurrency/SPSCQueue.hpp>
#include <lob/core/OrderRequest.hpp>
#include <lob/core/ExecutionReport.hpp>
#include <lob/network/gateway.hpp>


lob::concurrency::SPSCQueue<lob::core::OrderRequest, 1024> ingress_queue;
lob::concurrency::SPSCQueue<lob::core::ExecutionReport, 1024> egress_queue;

lob::memory::OrderPool pool(1000000);

lob::core::OrderBook book(20000, egress_queue);

// seperate this tests from tests that are threaded
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
// seperate this tests from tests that are threaded
void test_sell_and_buy() {

    lob::memory::OrderPool pool(1000000);
    lob::core::OrderBook book(20000, egress_queue);
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

void network_thread() {
    std::cout << "[NETWORK] Connecting to exchange...\n";
    
    // 1. Instantiate the Gateway, giving it the queue
    lob::network::Gateway gateway(ingress_queue);

    // 2. Simulate a raw byte buffer arriving from the OS network stack
    std::vector<uint8_t> fake_network_buffer;
    
    // manually construct two packed binary messages to simulate a TCP stream
    lob::network::ExchangePacket p1{'A', 101, 10050, 100, 'S'}; // Sell
    lob::network::ExchangePacket p2{'A', 102, 10050, 75, 'B'};  // Buy

    // Copy the raw bytes of these structs into our fake network buffer
    auto* p1_bytes = reinterpret_cast<uint8_t*>(&p1);
    auto* p2_bytes = reinterpret_cast<uint8_t*>(&p2);
    
    fake_network_buffer.insert(fake_network_buffer.end(), p1_bytes, p1_bytes + sizeof(p1));
    fake_network_buffer.insert(fake_network_buffer.end(), p2_bytes, p2_bytes + sizeof(p2));

    std::cout << "[NETWORK] Buffer of " << fake_network_buffer.size() << " bytes received. Passing to Gateway...\n";

    // 3. BLAST the raw bytes into the Gateway parser
    gateway.parse_tcp_buffer(fake_network_buffer.data(), fake_network_buffer.size());
}
    
void engine_thread() {
    lob::core::OrderRequest incoming_req;
    int processed = 0;

    // The core loop
    while (processed < 2) {
        if (ingress_queue.pop(incoming_req)) {
            uint32_t new_idx = pool.allocate();
            lob::core::Order& order = pool.get(new_idx);
            
            order.id = incoming_req.order_id;
            order.price = incoming_req.price;
            order.quantity = incoming_req.quantity;
            order.is_buy = incoming_req.is_buy;

            book.add_order(new_idx, pool);
            processed++;
        }
    }
}

void logger_thread() {
    lob::core::ExecutionReport report;
    int reports_received = 0;

    // We expect 1 trade to happen based on our simulated data
    while (reports_received < 1) {
        if (egress_queue.pop(report)) {
            // It is totally fine to use std::cout here because this thread 
            // is isolated and doesn't care about microsecond latency!
            std::cout << "[LOGGER] TRADE: Buy Order " << report.buy_id 
                      << " hit Sell Order " << report.sell_id 
                      << " for " << report.matched_quantity << " shares @ $" 
                      << report.matched_price / 100.0 << "\n";
            reports_received++;
        }
    }
}



int main() {
    //test_OrderPool_build();
    //test_sell_and_buy();

    // test produces and consumer threads -----
    std::cout << "--- Lock-Free Queue Test ---\n";

    // Launch threads
    std::thread network(network_thread);
    std::thread engine(engine_thread);
    std::thread egress(logger_thread); // egress logger thead

    // Wait for them to finish
    network.join();
    engine.join();
    egress.join();

    std::cout << "--- Test Complete ---\n";

    return 0;

}