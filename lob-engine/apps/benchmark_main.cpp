#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <x86intrin.h> // Gives us access to __rdtsc()
#include "lob/core/OrderBook.hpp"
#include "lob/memory/slab_allocator.hpp"
#include "lob/concurrency/SPSCQueue.hpp"

// Define the number of orders we want to test
constexpr size_t NUM_ORDERS = 1000000;
constexpr size_t WARMUP_RUNS = 100000; // First 100k runs warm up the CPU cache

int main() {
    std::cout << "--- Initializing Hardware Benchmark ---\n";

    // 1. Setup the Engine Infrastructure
    lob::concurrency::SPSCQueue<lob::core::ExecutionReport, 1024> egress_queue;
    lob::memory::OrderPool pool(2000000);
    lob::core::OrderBook book(20000, egress_queue);

    // 2. Pre-allocate dummy data to avoid memory noise during the test
    std::vector<uint32_t> resting_sells;
    std::vector<uint32_t> aggressive_buys;
    
    // We will measure the latency of exactly 1 million orders
    std::vector<uint64_t> latencies(NUM_ORDERS); 

    std::cout << "Building dummy orders in memory...\n";
    for (size_t i = 0; i < NUM_ORDERS; ++i) {
        // Create 1 million resting Sells at $100.50
        uint32_t sell_idx = pool.allocate();
        auto& sell = pool.get(sell_idx);
        sell.id = i;
        sell.price = 10050; 
        sell.quantity = 100;
        sell.is_buy = false;
        book.add_order(sell_idx, pool); // Load them into the book silently

        // Create 1 million aggressive Buys at $100.50 (guaranteed to cross/trade)
        uint32_t buy_idx = pool.allocate();
        auto& buy = pool.get(buy_idx);
        buy.id = NUM_ORDERS + i;
        buy.price = 10050; 
        buy.quantity = 100;
        buy.is_buy = true;
        aggressive_buys.push_back(buy_idx);
    }

    std::cout << "Firing " << NUM_ORDERS << " orders into the crossing engine...\n";

    // --- THE HOT PATH ---
    for (size_t i = 0; i < NUM_ORDERS; ++i) {
        
        // 1. Read hardware clock
        uint64_t start_cycles = __rdtsc(); 
        
        // 2. Execute the trade!
        // We use add_order directly to bypass the network queue for this specific benchmark
        book.add_order(aggressive_buys[i], pool); 
        
        // 3. Read hardware clock again
        uint64_t end_cycles = __rdtsc(); 
        
        // 4. Record the difference
        latencies[i] = end_cycles - start_cycles;
        
        // (Optional: Drain the egress queue so it doesn't fill up and block)
        lob::core::ExecutionReport report;
        while (egress_queue.pop(report)) {} 
    }
    // --- END HOT PATH ---

    std::cout << "Processing results...\n";

    // We throw away the first 100,000 iterations because the CPU cache was "cold".
    // In real HFT, the engine is always spinning, so the cache is always hot.
    auto valid_latencies_start = latencies.begin() + WARMUP_RUNS;
    std::vector<uint64_t> hot_latencies(valid_latencies_start, latencies.end());

    // Sort to calculate percentiles
    std::sort(hot_latencies.begin(), hot_latencies.end());

    size_t count = hot_latencies.size();
    uint64_t min_cycles = hot_latencies.front();
    uint64_t max_cycles = hot_latencies.back();
    uint64_t p50 = hot_latencies[count * 0.50];
    uint64_t p90 = hot_latencies[count * 0.90];
    uint64_t p99 = hot_latencies[count * 0.99];
    uint64_t p99_9 = hot_latencies[count * 0.999];

    // Average
    uint64_t sum = std::accumulate(hot_latencies.begin(), hot_latencies.end(), 0ULL);
    uint64_t avg = sum / count;

    // Output the Histogram
    std::cout << "\n=========================================\n";
    std::cout << "   CROSSING ENGINE LATENCY (IN CYCLES)   \n";
    std::cout << "=========================================\n";
    std::cout << "Average: " << avg << " cycles\n";
    std::cout << "Min:     " << min_cycles << " cycles\n";
    std::cout << "50th %:  " << p50 << " cycles (Median)\n";
    std::cout << "90th %:  " << p90 << " cycles\n";
    std::cout << "99th %:  " << p99 << " cycles\n";
    std::cout << "99.9th%: " << p99_9 << " cycles\n";
    std::cout << "Max:     " << max_cycles << " cycles\n";
    std::cout << "=========================================\n";

    return 0;
}