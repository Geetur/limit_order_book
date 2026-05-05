
#pragma once
#include <cstdint>

namespace lob::network {
// force compiler to ingest data with absolutely no padding
// i.e. this struct will always be parsed into 22 bytes
#pragma pack(push, 1)
// exchange packet describes the 22 bytes of a TCP stream
// we expect to jump 22 bytes on a stream, ingest, and jump again
struct ExchangePacket {
    char msg_type; // 1 byte: e.g. "A" for add order
    uint64_t order_id; // 8 bytes
    uint64_t price; // 8 bytes
    uint32_t quantity; // 4 bytes
    char side; // 1 byte: e.ge "B" for buy
};
// restore normal compiler padding
#pragma pack(pop)

};