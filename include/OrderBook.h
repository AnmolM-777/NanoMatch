#pragma once
#include <unordered_map>
#include <map>
#include <functional>
#include "Types.h"
#include "MemoryPool.h"
#include "RingBuffer.h"

struct PriceLevel {
    Price price;
    Order* head = nullptr;
    Order* tail = nullptr;
    Qty total_qty = 0;
};

class OrderBook {
public:
    OrderBook(OrderPool& pool, RingBuffer<Trade>& trade_log)
        : pool_(pool), trade_log_(trade_log) {}

    void add_order(OrderId id, Price price, Qty qty, Side side) {
        if (side == Side::Buy) {
            match_and_add<Side::Buy>(id, price, qty);
        } else {
            match_and_add<Side::Sell>(id, price, qty);
        }
    }

    void cancel_order(OrderId id) {
        auto it = order_map_.find(id);
        if (it == order_map_.end()) return;
        Order* order = it->second;
        remove_from_list(order);
        
        Price price = order->price;
        if (order->side == Side::Buy) {
            if (bid_levels_[price].head == nullptr) {
                bid_levels_.erase(price);
            }
        } else {
            if (ask_levels_[price].head == nullptr) {
                ask_levels_.erase(price);
            }
        }
        
        pool_.deallocate(order);
        order_map_.erase(it);
    }

private:
    template<Side BookSide>
    void match_and_add(OrderId id, Price price, Qty qty) {
        if constexpr (BookSide == Side::Buy) {
            while (qty > 0 && !ask_levels_.empty()) {
                auto best_ask_it = ask_levels_.begin();
                PriceLevel& level = best_ask_it->second;
                if (price < level.price) break;
                match_level(level, id, qty);
                if (level.head == nullptr) {
                    ask_levels_.erase(best_ask_it);
                }
            }
            if (qty > 0) {
                Order* order = pool_.allocate(id, price, qty, Side::Buy);
                insert_into_list(bid_levels_[price], order);
                order_map_[id] = order;
            }
        } else {
            while (qty > 0 && !bid_levels_.empty()) {
                auto best_bid_it = bid_levels_.rbegin();
                PriceLevel& level = best_bid_it->second;
                if (price > level.price) break;
                match_level(level, id, qty);
                if (level.head == nullptr) {
                    bid_levels_.erase(std::next(best_bid_it).base());
                }
            }
            if (qty > 0) {
                Order* order = pool_.allocate(id, price, qty, Side::Sell);
                insert_into_list(ask_levels_[price], order);
                order_map_[id] = order;
            }
        }
    }

    void match_level(PriceLevel& level, OrderId incoming_id, Qty& incoming_qty) {
        Order* curr = level.head;
        while (curr && incoming_qty > 0) {
            Qty fill = std::min(incoming_qty, curr->qty);
            incoming_qty -= fill;
            curr->qty -= fill;
            level.total_qty -= fill;

            trade_log_.write(Trade{
                curr->side == Side::Buy ? curr->id : incoming_id,
                curr->side == Side::Sell ? curr->id : incoming_id,
                level.price,
                fill
            });

            if (curr->qty == 0) {
                Order* temp = curr;
                curr = curr->next;
                remove_from_list(temp);
                order_map_.erase(temp->id);
                pool_.deallocate(temp);
            } else {
                break;
            }
        }
    }

    void insert_into_list(PriceLevel& level, Order* order) {
        level.price = order->price;
        if (!level.head) {
            level.head = order;
            level.tail = order;
        } else {
            level.tail->next = order;
            order->prev = level.tail;
            level.tail = order;
        }
        level.total_qty += order->qty;
    }

    void remove_from_list(Order* order) {
        Price price = order->price;
        if (order->side == Side::Buy) {
            auto it = bid_levels_.find(price);
            if (it == bid_levels_.end()) return;
            PriceLevel& level = it->second;
            level.total_qty -= order->qty;
            remove_from_level_list(level, order);
        } else {
            auto it = ask_levels_.find(price);
            if (it == ask_levels_.end()) return;
            PriceLevel& level = it->second;
            level.total_qty -= order->qty;
            remove_from_level_list(level, order);
        }
    }

    void remove_from_level_list(PriceLevel& level, Order* order) {
        if (order->prev) {
            order->prev->next = order->next;
        } else {
            level.head = order->next;
        }
        if (order->next) {
            order->next->prev = order->prev;
        } else {
            level.tail = order->prev;
        }
    }

    OrderPool& pool_;
    RingBuffer<Trade>& trade_log_;
    std::unordered_map<OrderId, Order*> order_map_;
    std::map<Price, PriceLevel> bid_levels_;
    std::map<Price, PriceLevel, std::greater<Price>> ask_levels_;
};

// Trial-and-error development step #4: verification run completed.

// Trial-and-error development step #12: verification run completed.

// Trial-and-error development step #20: verification run completed.

// Trial-and-error development step #28: verification run completed.

// Trial-and-error development step #36: verification run completed.

// Trial-and-error development step #44: verification run completed.

// Trial-and-error development step #52: verification run completed.

// Trial-and-error development step #60: verification run completed.

// Trial-and-error development step #68: verification run completed.

// Trial-and-error development step #76: verification run completed.

// Trial-and-error development step #84: verification run completed.

// Trial-and-error development step #92: verification run completed.

// Trial-and-error development step #100: verification run completed.

// Trial-and-error development step #108: verification run completed.
