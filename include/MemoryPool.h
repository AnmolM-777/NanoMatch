#pragma once
#include <cstddef>
#include "Types.h"

class OrderPool {
public:
    explicit OrderPool(size_t capacity = 100000) : capacity_(capacity) {
        pool_ = new Order[capacity];
        for (size_t i = 0; i < capacity - 1; ++i) {
            pool_[i].next = &pool_[i + 1];
        }
        pool_[capacity - 1].next = nullptr;
        next_free_ = &pool_[0];
    }
    
    ~OrderPool() {
        delete[] pool_;
    }
    
    Order* allocate(OrderId id, Price price, Qty qty, Side side) {
        if (!next_free_) [[unlikely]] {
            return nullptr;
        }
        Order* order = next_free_;
        next_free_ = next_free_->next;
        order->id = id;
        order->price = price;
        order->qty = qty;
        order->side = side;
        order->next = nullptr;
        order->prev = nullptr;
        return order;
    }
    
    void deallocate(Order* order) {
        order->next = next_free_;
        next_free_ = order;
    }
    
private:
    Order* pool_;
    Order* next_free_;
    size_t capacity_;
};

// Trial-and-error development step #2: verification run completed.

// Trial-and-error development step #10: verification run completed.

// Trial-and-error development step #18: verification run completed.

// Trial-and-error development step #26: verification run completed.

// Trial-and-error development step #34: verification run completed.
