#pragma once
#include <cstdint>
#include <cstddef>
#include <emmintrin.h>
#include "lob/concurrency/SPSCQueue.hpp"
#include "lob/core/OrderRequest.hpp"
#include "lob/network/ExchangePacket.hpp"

// NIC → kernel ring buffer → TCP stack → syscall (recv/read) → this gateway
// expected latency of this is ~50 microseconds
namespace lob::network {

    class Gateway{

    private:

        lob::concurrency::SPSCQueue<lob::core::OrderRequest, 1024>& ingress_queue;
    
    public:
        
        explicit Gateway(lob::concurrency::SPSCQueue<lob::core::OrderRequest, 1024>& ingress_queue) : 
        ingress_queue(ingress_queue) {}

        // intake raw tcp stream and its length
        inline void parse_tcp_buffer(const uint8_t* raw_buffer, size_t length) {

            // offset can also be defined as count * sizeof(exchange_packet)
            // however the multiplication operation is more expensive than simply adding
            // sizeof each iteration
            size_t offset = 0;

            // could potentially pre-compute sizeof packet and
            // const chars such as 'A'
            while (offset + sizeof(ExchangePacket) <= length) {
                // map the struct directly onto the raw stream
                const ExchangePacket* packet = reinterpret_cast<const ExchangePacket*>(raw_buffer + offset);

                // add order
                if (packet->msg_type == 'A') {

                    lob::core::OrderRequest order;
                    order.order_id = packet->order_id;
                    order.price = packet->price;
                    order.quantity = packet->quantity;
                    order.is_buy = (packet->side == 'B');

                    while (!ingress_queue.push(order)) {
                        _mm_pause();
                    }

                }

                offset += sizeof(ExchangePacket);
            }
        }
    };
}