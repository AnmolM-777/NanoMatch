#include <iostream>
#include <vector>
#include <cassert>
#include "Types.h"
#include "MemoryPool.h"
#include "RingBuffer.h"
#include "STLOrderBook.h"
#include "OptimizedOrderBook.h"

int main() {
    std::cout << "=== NanoMatch Verification Test ===" << std::endl;

    OrderPool pool(100000);
    RingBuffer<Trade> stl_log;
    RingBuffer<Trade> opt_log;

    STLOrderBook stl_book(stl_log);
    OptimizedOrderBook opt_book(pool, opt_log);

    struct OrderInput {
        OrderId id;
        Price price;
        Qty qty;
        Side side;
        bool is_cancel;
    };

    std::vector<OrderInput> inputs = {
        {1, 10050, 100, Side::Buy, false},
        {2, 10040, 50, Side::Buy, false},
        {3, 10050, 120, Side::Sell, false}, // Match 100 @ 10050 (Buy 1 & Sell 3)
        {4, 10030, 80, Side::Sell, false},  // Match 50 @ 10040 (Buy 2 & Sell 4)
        {5, 10060, 40, Side::Sell, false},  // Resting ask
        {6, 10070, 60, Side::Sell, false},  // Resting ask
        {7, 10065, 50, Side::Buy, false},   // Resting bid
        {8, 10070, 70, Side::Buy, false},   // Match 40 @ 10060 (Buy 8 & Sell 5), then Match 30 @ 10070 (Buy 8 & Sell 6)
        {2, 0, 0, Side::Buy, true},         // Cancel Order 2 (already filled, safe no-op)
        {7, 0, 0, Side::Buy, true}          // Cancel Order 7 (removes from book)
    };

    // Feed to STL
    for (const auto& op : inputs) {
        if (op.is_cancel) {
            stl_book.cancel_order(op.id);
        } else {
            stl_book.add_order(op.id, op.price, op.qty, op.side);
        }
    }

    // Feed to Optimized
    for (const auto& op : inputs) {
        if (op.is_cancel) {
            opt_book.cancel_order(op.id);
        } else {
            opt_book.add_order(op.id, op.price, op.qty, op.side);
        }
    }

    // Collect and compare trades
    std::vector<Trade> stl_trades;
    Trade t;
    while (stl_log.read(t)) {
        stl_trades.push_back(t);
    }

    std::vector<Trade> opt_trades;
    while (opt_log.read(t)) {
        opt_trades.push_back(t);
    }

    std::cout << "[VERIFICATION] STL trades matched: " << stl_trades.size() << std::endl;
    std::cout << "[VERIFICATION] Optimized trades matched: " << opt_trades.size() << std::endl;

    bool match = (stl_trades.size() == opt_trades.size());
    if (match) {
        for (size_t i = 0; i < stl_trades.size(); ++i) {
            if (stl_trades[i].buy_id != opt_trades[i].buy_id ||
                stl_trades[i].sell_id != opt_trades[i].sell_id ||
                stl_trades[i].price != opt_trades[i].price ||
                stl_trades[i].qty != opt_trades[i].qty) {
                match = false;
                std::cout << "[MISMATCH] Trade #" << i << " differs!" << std::endl;
                std::cout << "  STL: Buy #" << stl_trades[i].buy_id << " Sell #" << stl_trades[i].sell_id << " Price " << stl_trades[i].price << " Qty " << stl_trades[i].qty << std::endl;
                std::cout << "  OPT: Buy #" << opt_trades[i].buy_id << " Sell #" << opt_trades[i].sell_id << " Price " << opt_trades[i].price << " Qty " << opt_trades[i].qty << std::endl;
            }
        }
    }

    if (match) {
        std::cout << "[VERIFICATION SUCCESS] STL and Optimized OrderBook matched perfectly!" << std::endl;
    } else {
        std::cout << "[VERIFICATION FAILED] Mismatch detected between STL and Optimized!" << std::endl;
        return 1;
    }

    return 0;
}
