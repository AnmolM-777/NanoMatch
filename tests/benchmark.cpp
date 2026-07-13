#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <random>
#include <iomanip>
#include "Types.h"
#include "MemoryPool.h"
#include "RingBuffer.h"
#include "STLOrderBook.h"
#include "OptimizedOrderBook.h"

// High-resolution timer helper in nanoseconds
static inline uint64_t get_time_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

#ifdef __x86_64__
static inline uint64_t rdtsc() {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}
#elif defined(__aarch64__)
static inline uint64_t rdtsc() {
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0" : "=r" (val));
    return val;
}
#else
static inline uint64_t rdtsc() {
    return get_time_ns();
}
#endif

struct BenchmarkOrder {
    OrderId id;
    Price price;
    Qty qty;
    Side side;
    bool is_cancel;
};

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "       NANOMATCH PERFORMANCE BENCHMARK SUITE       " << std::endl;
    std::cout << "==================================================" << std::endl;

    const int num_orders = 500000;
    std::vector<BenchmarkOrder> orders;
    orders.reserve(num_orders);

    std::mt19937 rng(42);
    // Price range: 9900 to 10100 (well within our array range)
    std::uniform_int_distribution<Price> price_dist(9900, 10100);
    std::uniform_int_distribution<Qty> qty_dist(1, 100);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> action_dist(0, 9); // 10% cancellation rate

    std::vector<OrderId> active_ids;
    active_ids.reserve(num_orders);

    OrderId current_id = 1;
    for (int i = 0; i < num_orders; ++i) {
        // Cancel an order with 10% probability, if we have active orders
        if (action_dist(rng) == 0 && !active_ids.empty()) {
            size_t idx = std::uniform_int_distribution<size_t>(0, active_ids.size() - 1)(rng);
            OrderId cancel_id = active_ids[idx];
            orders.push_back({cancel_id, 0, 0, Side::Buy, true});
            // Remove from active_ids
            active_ids[idx] = active_ids.back();
            active_ids.pop_back();
        } else {
            Price p = price_dist(rng);
            Qty q = qty_dist(rng);
            Side s = side_dist(rng) ? Side::Buy : Side::Sell;
            orders.push_back({current_id, p, q, s, false});
            active_ids.push_back(current_id);
            current_id++;
        }
    }

    std::cout << "Generated " << num_orders << " simulated order book events." << std::endl;
    std::cout << "Starting benchmark runs..." << std::endl;

    // ----------------------------------------------------
    // Baseline STL Benchmark Run
    // ----------------------------------------------------
    {
        RingBuffer<Trade> stl_log;
        STLOrderBook stl_book(stl_log);

        // Warm up
        for (int i = 0; i < 5000; ++i) {
            stl_book.add_order(i + 10000000, 10000, 10, Side::Buy);
        }

        std::vector<uint64_t> latencies_ns;
        latencies_ns.reserve(num_orders);

        uint64_t start_total = get_time_ns();
        for (const auto& op : orders) {
            uint64_t start = get_time_ns();
            if (op.is_cancel) {
                stl_book.cancel_order(op.id);
            } else {
                stl_book.add_order(op.id, op.price, op.qty, op.side);
            }
            uint64_t end = get_time_ns();
            latencies_ns.push_back(end - start);
        }
        uint64_t end_total = get_time_ns();

        double elapsed_seconds = (end_total - start_total) / 1e9;
        double throughput = num_orders / elapsed_seconds;

        std::sort(latencies_ns.begin(), latencies_ns.end());
        uint64_t p50 = latencies_ns[num_orders * 0.5];
        uint64_t p90 = latencies_ns[num_orders * 0.9];
        uint64_t p99 = latencies_ns[num_orders * 0.99];
        uint64_t p999 = latencies_ns[num_orders * 0.999];

        std::cout << "\n--- Baseline STL OrderBook Results ---" << std::endl;
        std::cout << "  Throughput: " << std::fixed << std::setprecision(2) << throughput << " orders/sec" << std::endl;
        std::cout << "  Latencies (nanoseconds):" << std::endl;
        std::cout << "    p50 (median): " << p50 << " ns" << std::endl;
        std::cout << "    p90:          " << p90 << " ns" << std::endl;
        std::cout << "    p99 (tail):   " << p99 << " ns" << std::endl;
        std::cout << "    p99.9:        " << p999 << " ns" << std::endl;
    }

    // ----------------------------------------------------
    // Optimized Benchmark Run
    // ----------------------------------------------------
    {
        OrderPool pool(num_orders + 10000);
        RingBuffer<Trade> opt_log;
        OptimizedOrderBook opt_book(pool, opt_log);

        // Warm up
        for (int i = 0; i < 5000; ++i) {
            opt_book.add_order(i + 10000000, 10000, 10, Side::Buy);
        }

        std::vector<uint64_t> latencies_ns;
        latencies_ns.reserve(num_orders);

        uint64_t start_total = get_time_ns();
        for (const auto& op : orders) {
            uint64_t start = get_time_ns();
            if (op.is_cancel) {
                opt_book.cancel_order(op.id);
            } else {
                opt_book.add_order(op.id, op.price, op.qty, op.side);
            }
            uint64_t end = get_time_ns();
            latencies_ns.push_back(end - start);
        }
        uint64_t end_total = get_time_ns();

        double elapsed_seconds = (end_total - start_total) / 1e9;
        double throughput = num_orders / elapsed_seconds;

        std::sort(latencies_ns.begin(), latencies_ns.end());
        uint64_t p50 = latencies_ns[num_orders * 0.5];
        uint64_t p90 = latencies_ns[num_orders * 0.9];
        uint64_t p99 = latencies_ns[num_orders * 0.99];
        uint64_t p999 = latencies_ns[num_orders * 0.999];

        std::cout << "\n--- Optimized OrderBook Results ---" << std::endl;
        std::cout << "  Throughput: " << std::fixed << std::setprecision(2) << throughput << " orders/sec" << std::endl;
        std::cout << "  Latencies (nanoseconds):" << std::endl;
        std::cout << "    p50 (median): " << p50 << " ns" << std::endl;
        std::cout << "    p90:          " << p90 << " ns" << std::endl;
        std::cout << "    p99 (tail):   " << p99 << " ns" << std::endl;
        std::cout << "    p99.9:        " << p999 << " ns" << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;
    }

    return 0;
}
