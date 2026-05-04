# pragma once

# include <atomic>
# include <cstdint>
# include <cstddef>
# include <stdexcept>

namespace lob::concurrency {

    constexpr size_t CACHE_LINE_SIZE = 64;

    
    template<typename T, size_t Size> 
    class SPSCQueue {
        // must be a power of two to allow for fast modulo operation via AND
        // statically assert during compile time
        static_assert((Size != 0) && ((Size & (Size - 1)) == 0), "Queue size must be a power of two");

    private:
    
        // for fast modulo
        static constexpr size_t MASK = Size - 1;

        // Force head and tail to be on seperate cache lines. This prevents false sharing
        // between producer and consumer threads that would slow down this queue
        alignas(CACHE_LINE_SIZE) std::atomic<size_t> head{0};
        alignas(CACHE_LINE_SIZE) std::atomic<size_t> tail{0};
        
        // underlying array
        T buffer[Size];

    public:

        SPSCQueue() = default;

        // PRODUCER THREAD              CONSUMER THREAD
        // ──────────────────           ──────────────────
        // buffer[t] = item;            
        // tail.store(release)   ──→    tail.load(acquire)
        //                              item = buffer[t]  ← guaranteed to see the write above

        // only ever called by the producer thread (ingress)
        inline bool push(const T& item) {

            // memory order can be relaxed because only this thread ever
            // interacts with the current tail. the producer thread owns the tail
            const size_t curr_tail = tail.load(std::memory_order_relaxed) ;
            const size_t next_tail = (curr_tail + 1) & MASK;

            // memory order acquire ensures we are seeing the most recent head
            // updated by the consumer. If true the queue is full and we return false
            // in other words we cant load without confirming at some point it was released
            if (next_tail == head.load(std::memory_order_acquire)) [[unlikely]] {
                return false;
            }

            // if not true then we can update the buffer
            buffer[curr_tail] = item;
            // memory order release ensures that data is written fully before the tail updates
            // and so it's impossible for the consumer thread to see a new tail but old data
            tail.store(next_tail, std::memory_order_release);

            return true;

        }

        inline bool pop(T& item) {

            // consumer thread completely owns the head
            const size_t curr_head = head.load(std::memory_order_relaxed);

            // synchronizes with the producers release
            if (curr_head == tail.load(std::memory_order_acquire)) [[unlikely]] {
                // queue is empty
                return false;
            }

            // copying the data from the buffer into the item variable
            item = buffer[curr_head];

            // moves the head forward, and once again guaranteeing that
            // before the head updates the data has been fully moved out of the buffer
            // signaling to the producer that the previous head slot can be overwritten
            head.store((curr_head + 1) & MASK, std::memory_order_release);

            return true;


        }


    };



}