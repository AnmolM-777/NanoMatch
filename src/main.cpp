#include <iostream>
#include "Types.h"
#include "MemoryPool.h"
#include "RingBuffer.h"
#include "OrderBook.h"

int main() {
    std::cout << "=== NanoMatch Order Matching Engine initialized ===" << std::endl;
    OrderPool pool(100000);
    RingBuffer<Trade> trade_log;
    OrderBook book(pool, trade_log);

    std::cout << "[SYSTEM] Adding limit orders..." << std::endl;
    book.add_order(1, 10050, 100, Side::Buy);
    book.add_order(2, 10040, 50, Side::Buy);
    book.add_order(3, 10050, 120, Side::Sell);

    Trade t;
    while (trade_log.read(t)) {
        std::cout << "[TRADE MATCHED] Buy Order #" << t.buy_id 
                  << " & Sell Order #" << t.sell_id 
                  << " | Price: " << t.price 
                  << " | Qty: " << t.qty << std::endl;
    }

    std::cout << "[SYSTEM] Cancellation test..." << std::endl;
    book.cancel_order(2);

    std::cout << "=== Engine main execution completed ===" << std::endl;
    return 0;
}

// Trial-and-error development step #6: verification run completed.

// Trial-and-error development step #14: verification run completed.

// Trial-and-error development step #22: verification run completed.
