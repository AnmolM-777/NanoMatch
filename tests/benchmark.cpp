#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <random>
#include "Types.h"
#include "MemoryPool.h"
#include "RingBuffer.h"
#include "OrderBook.h"

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
    return std::chrono::steady_clock::now().time_since_epoch().count();
}
#endif

int main() {
    std::cout << "=== NanoMatch Performance Benchmark ===" << std::endl;
    OrderPool pool(1000000);
    RingBuffer<Trade> trade_log;
    OrderBook book(pool, trade_log);

    const int num_orders = 100000;
    std::vector<uint64_t> latencies;
    latencies.reserve(num_orders);

    std::mt19937 rng(42);
    std::uniform_int_distribution<Price> price_dist(9900, 10100);
    std::uniform_int_distribution<Qty> qty_dist(1, 100);
    std::uniform_int_distribution<int> side_dist(0, 1);

    // Warm up
    for (int i = 0; i < 1000; ++i) {
        book.add_order(i, price_dist(rng), qty_dist(rng), side_dist(rng) ? Side::Buy : Side::Sell);
    }

    // Benchmark
    for (int i = 1000; i < num_orders + 1000; ++i) {
        Price p = price_dist(rng);
        Qty q = qty_dist(rng);
        Side s = side_dist(rng) ? Side::Buy : Side::Sell;
        
        uint64_t start = rdtsc();
        book.add_order(i, p, q, s);
        uint64_t end = rdtsc();
        
        latencies.push_back(end - start);
    }

    std::sort(latencies.begin(), latencies.end());

    double ticks_p50 = latencies[num_orders * 0.5];
    double ticks_p90 = latencies[num_orders * 0.9];
    double ticks_p99 = latencies[num_orders * 0.99];

    std::cout << "Processed " << num_orders << " orders." << std::endl;
    std::cout << "Latency Percentiles (in TSC ticks):" << std::endl;
    std::cout << "  p50:   " << ticks_p50 << " ticks" << std::endl;
    std::cout << "  p90:   " << ticks_p90 << " ticks" << std::endl;
    std::cout << "  p99:   " << ticks_p99 << " ticks" << std::endl;

    return 0;
}

// Trial-and-error development step #7: verification run completed.
