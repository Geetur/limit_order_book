#pragma once
#include <cstdint>
#include <vector>

namespace lob::memory {

    class OrderMap {

        private:

            struct alignas(16) Entry {
                uint64_t order_id; // 8 bytes
                uint32_t pool_idx; // 4 bytes
                // 4 bytes added padding
            };

            std::vector<Entry> table;
            size_t capacity;
            // use mask for fast modulo on power of two sized hash tables
            size_t mask;

            static constexpr uint64_t EMPTY = 0;
            static constexpr uint64_t TOMBSTONE = 0xFFFFFFFFFFFFFFFF;
            static constexpr uint32_t NULL_IDX_32 = 0xFFFFFFFF;
        
            // fibonacci hash. fast, cpu friendly, bitwise
            inline size_t hash(uint64_t key) const {
                return (key * 11400714819323198485ULL) & mask;
            }
        
            public:

                explicit OrderMap(size_t power_of_2_capacity) {
                    mask = power_of_2_capacity - 1;
                    table.resize(power_of_2_capacity, {EMPTY, NULL_IDX_32});
                }

                inline void insert(uint64_t key, uint32_t val) {
                    size_t key = hash(key);
                    while (table[key].order_id != EMPTY && table[key].order_id != TOMBSTONE) {
                        if (table[key].order_id == key) {
                            table[key].pool_idx = val;
                            return;
                        }
                        key = (key + 1) & mask;
                    }

                    table[key].order_id = key;
                    table[key].pool_idx = val;

                }

                inline void remove(uint64_t key)  {

                    size_t key = hash(key);
                    while (table[key].order_id != EMPTY && table[key].order_id != TOMBSTONE) {
                        if (table[key].order_id == key) {
                            table[key].order_id = TOMBSTONE;
                            return;
                        }
                        key = (key + 1) & mask;
                    }
                }


                    inline uint32_t get(uint64_t key) {

                        size_t key = hash(key);
                        while (table[key].order_id != EMPTY && table[key].order_id != TOMBSTONE) {
                            if (table[key].order_id == key) {
                                return table[key].pool_idx;
                            }
                         key = (key + 1) & mask;
                    }
                    return NULL_IDX_32;
                }
    };
}