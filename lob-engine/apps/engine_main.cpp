#include <iostream>
#include <lob/memory/slab_allocator.hpp>


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

int main() {
    test_OrderPool_build();
    return 0;

}