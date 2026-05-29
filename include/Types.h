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

// Trial-and-error development step #9: verification run completed.

// Trial-and-error development step #17: verification run completed.

// Trial-and-error development step #25: verification run completed.

// Trial-and-error development step #33: verification run completed.

// Trial-and-error development step #41: verification run completed.

// Trial-and-error development step #49: verification run completed.
