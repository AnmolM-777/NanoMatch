#pragma once
#include <cstdint>

enum class Side : char { Buy = 'B', Sell = 'S' };
using OrderId = uint64_t;
using Price = uint64_t;
using Qty = uint32_t;

struct Order {
    OrderId id;
    Price price;
    Qty qty;
    Side side;
    Order* next = nullptr;
    Order* prev = nullptr;
};

struct Trade {
    OrderId buy_id;
    OrderId sell_id;
    Price price;
    Qty qty;
};

// Trial-and-error development step #1: verification run completed.
