#pragma once
#include <vector>
#include <limits>
#include <algorithm>
#include <iostream>
#include "Types.h"
#include "MemoryPool.h"
#include "RingBuffer.h"

class OptimizedOrderBook {
public:
    static constexpr Price MAX_PRICE = 2000000;

    OptimizedOrderBook(OrderPool& pool, RingBuffer<Trade>& trade_log)
        : pool_(pool), trade_log_(trade_log) {
        bid_levels_ = new PriceLevel[MAX_PRICE]();
        ask_levels_ = new PriceLevel[MAX_PRICE]();
        // preallocate order_map_ for fast lookups
        order_map_.resize(1000000, nullptr);
    }

    ~OptimizedOrderBook() {
        delete[] bid_levels_;
        delete[] ask_levels_;
    }

    void add_order(OrderId id, Price price, Qty qty, Side side) {
        if (price >= MAX_PRICE) [[unlikely]] {
            std::cerr << "[ERROR] Price " << price << " exceeds MAX_PRICE limit." << std::endl;
            return;
        }

        if (side == Side::Buy) {
            match_and_add_buy(id, price, qty);
        } else {
            match_and_add_sell(id, price, qty);
        }
    }

    void cancel_order(OrderId id) {
        if (id >= order_map_.size() || !order_map_[id]) return;
        Order* order = order_map_[id];
        Price price = order->price;
        Side side = order->side;

        if (side == Side::Buy) {
            PriceLevel& level = bid_levels_[price];
            level.total_qty -= order->qty;
            remove_from_level_list(level, order);
            if (level.head == nullptr) {
                if (price == best_bid_price_) {
                    update_best_bid();
                }
            }
        } else {
            PriceLevel& level = ask_levels_[price];
            level.total_qty -= order->qty;
            remove_from_level_list(level, order);
            if (level.head == nullptr) {
                if (price == best_ask_price_) {
                    update_best_ask();
                }
            }
        }

        pool_.deallocate(order);
        order_map_[id] = nullptr;
    }

private:
    void match_and_add_buy(OrderId id, Price price, Qty qty) {
        // Match against asks
        while (qty > 0 && best_ask_price_ <= price) {
            PriceLevel& level = ask_levels_[best_ask_price_];
            Order* curr = level.head;
            while (curr && qty > 0) {
                Qty fill = std::min(qty, curr->qty);
                qty -= fill;
                curr->qty -= fill;
                level.total_qty -= fill;

                trade_log_.write(Trade{
                    id, // Buy
                    curr->id, // Sell
                    best_ask_price_,
                    fill
                });

                if (curr->qty == 0) {
                    Order* temp = curr;
                    curr = curr->next;
                    remove_from_level_list(level, temp);
                    order_map_[temp->id] = nullptr;
                    pool_.deallocate(temp);
                } else {
                    break;
                }
            }

            if (level.head == nullptr) {
                update_best_ask();
            } else {
                break; // If level not fully filled, the qty must be 0
            }
        }

        if (qty > 0) {
            Order* order = pool_.allocate(id, price, qty, Side::Buy);
            if (id >= order_map_.size()) [[unlikely]] {
                order_map_.resize(id * 2, nullptr);
            }
            order_map_[id] = order;
            insert_into_level_list(bid_levels_[price], order);
            if (price > best_bid_price_) {
                best_bid_price_ = price;
            }
        }
    }

    void match_and_add_sell(OrderId id, Price price, Qty qty) {
        // Match against bids
        while (qty > 0 && best_bid_price_ >= price) {
            PriceLevel& level = bid_levels_[best_bid_price_];
            Order* curr = level.head;
            while (curr && qty > 0) {
                Qty fill = std::min(qty, curr->qty);
                qty -= fill;
                curr->qty -= fill;
                level.total_qty -= fill;

                trade_log_.write(Trade{
                    curr->id, // Buy
                    id, // Sell
                    best_bid_price_,
                    fill
                });

                if (curr->qty == 0) {
                    Order* temp = curr;
                    curr = curr->next;
                    remove_from_level_list(level, temp);
                    order_map_[temp->id] = nullptr;
                    pool_.deallocate(temp);
                } else {
                    break;
                }
            }

            if (level.head == nullptr) {
                update_best_bid();
            } else {
                break;
            }
        }

        if (qty > 0) {
            Order* order = pool_.allocate(id, price, qty, Side::Sell);
            if (id >= order_map_.size()) [[unlikely]] {
                order_map_.resize(id * 2, nullptr);
            }
            order_map_[id] = order;
            insert_into_level_list(ask_levels_[price], order);
            if (price < best_ask_price_) {
                best_ask_price_ = price;
            }
        }
    }

    void insert_into_level_list(PriceLevel& level, Order* order) {
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
        order->next = nullptr;
        order->prev = nullptr;
    }

    void update_best_bid() {
        while (best_bid_price_ > 0 && !bid_levels_[best_bid_price_].head) {
            best_bid_price_--;
        }
    }

    void update_best_ask() {
        while (best_ask_price_ < MAX_PRICE && !ask_levels_[best_ask_price_].head) {
            best_ask_price_++;
        }
    }

    OrderPool& pool_;
    RingBuffer<Trade>& trade_log_;
    PriceLevel* bid_levels_;
    PriceLevel* ask_levels_;
    Price best_bid_price_ = 0;
    Price best_ask_price_ = MAX_PRICE;
    std::vector<Order*> order_map_;
};
