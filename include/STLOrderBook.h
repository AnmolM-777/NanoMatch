#pragma once
#include <map>
#include <list>
#include <unordered_map>
#include "Types.h"
#include "RingBuffer.h"

struct STLOrder {
    OrderId id;
    Price price;
    Qty qty;
    Side side;
};

class STLOrderBook {
public:
    explicit STLOrderBook(RingBuffer<Trade>& trade_log) : trade_log_(trade_log) {}

    void add_order(OrderId id, Price price, Qty qty, Side side) {
        if (side == Side::Buy) {
            match_and_add_buy(id, price, qty);
        } else {
            match_and_add_sell(id, price, qty);
        }
    }

    void cancel_order(OrderId id) {
        auto it = order_map_.find(id);
        if (it == order_map_.end()) return;

        auto [price, list_it] = it->second;
        Side side = list_it->side;

        if (side == Side::Buy) {
            auto level_it = bid_levels_.find(price);
            if (level_it != bid_levels_.end()) {
                level_it->second.erase(list_it);
                if (level_it->second.empty()) {
                    bid_levels_.erase(level_it);
                }
            }
        } else {
            auto level_it = ask_levels_.find(price);
            if (level_it != ask_levels_.end()) {
                level_it->second.erase(list_it);
                if (level_it->second.empty()) {
                    ask_levels_.erase(level_it);
                }
            }
        }
        order_map_.erase(it);
    }

private:
    void match_and_add_buy(OrderId id, Price price, Qty qty) {
        while (qty > 0 && !ask_levels_.empty()) {
            auto best_ask_it = ask_levels_.begin();
            Price best_ask_price = best_ask_it->first;
            if (price < best_ask_price) break;

            auto& level_orders = best_ask_it->second;
            auto order_it = level_orders.begin();
            while (order_it != level_orders.end() && qty > 0) {
                Qty fill = std::min(qty, order_it->qty);
                qty -= fill;
                order_it->qty -= fill;

                trade_log_.write(Trade{
                    id, // Buy
                    order_it->id, // Sell
                    best_ask_price,
                    fill
                });

                if (order_it->qty == 0) {
                    order_map_.erase(order_it->id);
                    order_it = level_orders.erase(order_it);
                } else {
                    break;
                }
            }

            if (level_orders.empty()) {
                ask_levels_.erase(best_ask_it);
            }
        }

        if (qty > 0) {
            auto& level_orders = bid_levels_[price];
            level_orders.push_back(STLOrder{id, price, qty, Side::Buy});
            order_map_[id] = {price, std::prev(level_orders.end())};
        }
    }

    void match_and_add_sell(OrderId id, Price price, Qty qty) {
        while (qty > 0 && !bid_levels_.empty()) {
            auto best_bid_it = bid_levels_.begin(); // sorted descending, so begin() is highest bid price
            Price best_bid_price = best_bid_it->first;
            if (price > best_bid_price) break;

            auto& level_orders = best_bid_it->second;
            auto order_it = level_orders.begin();
            while (order_it != level_orders.end() && qty > 0) {
                Qty fill = std::min(qty, order_it->qty);
                qty -= fill;
                order_it->qty -= fill;

                trade_log_.write(Trade{
                    order_it->id, // Buy
                    id, // Sell
                    best_bid_price,
                    fill
                });

                if (order_it->qty == 0) {
                    order_map_.erase(order_it->id);
                    order_it = level_orders.erase(order_it);
                } else {
                    break;
                }
            }

            if (level_orders.empty()) {
                bid_levels_.erase(best_bid_it);
            }
        }

        if (qty > 0) {
            auto& level_orders = ask_levels_[price];
            level_orders.push_back(STLOrder{id, price, qty, Side::Sell});
            order_map_[id] = {price, std::prev(level_orders.end())};
        }
    }

    RingBuffer<Trade>& trade_log_;
    std::map<Price, std::list<STLOrder>, std::greater<Price>> bid_levels_;
    std::map<Price, std::list<STLOrder>, std::less<Price>> ask_levels_;
    std::unordered_map<OrderId, std::pair<Price, std::list<STLOrder>::iterator>> order_map_;
};
