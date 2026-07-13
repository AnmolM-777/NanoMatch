#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <iomanip>
#include "Types.h"
#include "MemoryPool.h"
#include "RingBuffer.h"
#include "STLOrderBook.h"
#include "OptimizedOrderBook.h"
#include "Parser.h"

int main(int argc, char* argv[]) {
    std::string csv_path = "tests/data.csv";
    if (argc >= 2) {
        csv_path = argv[1];
    }

    std::cout << "==================================================" << std::endl;
    std::cout << "       NANOMATCH DATASET MATCHING RUNNER          " << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Target CSV dataset: " << csv_path << std::endl;

    // Run STL
    std::vector<Trade> stl_trades;
    {
        RingBuffer<Trade> stl_log;
        STLOrderBook stl_book(stl_log);
        
        std::cout << "[SYSTEM] Processing dataset with STL OrderBook (Baseline)..." << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        CSVParser::parse_file(csv_path, stl_book);
        auto end = std::chrono::high_resolution_clock::now();
        
        Trade t;
        while (stl_log.read(t)) {
            stl_trades.push_back(t);
        }
        
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        double elapsed_seconds = duration_ms / 1000.0;
        double throughput = 1000000.0 / elapsed_seconds;
        
        std::cout << "  Elapsed Time: " << duration_ms << " ms" << std::endl;
        std::cout << "  Throughput:   " << std::fixed << std::setprecision(2) << throughput << " events/sec" << std::endl;
        std::cout << "  Total Trades: " << stl_trades.size() << std::endl;
    }

    // Run Optimized
    std::vector<Trade> opt_trades;
    {
        OrderPool pool(10000000); // Preallocate 10M arena
        RingBuffer<Trade> opt_log;
        OptimizedOrderBook opt_book(pool, opt_log);
        
        std::cout << "\n[SYSTEM] Processing dataset with Optimized OrderBook..." << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        CSVParser::parse_file(csv_path, opt_book);
        auto end = std::chrono::high_resolution_clock::now();
        
        Trade t;
        while (opt_log.read(t)) {
            opt_trades.push_back(t);
        }
        
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        double elapsed_seconds = duration_ms / 1000.0;
        double throughput = 1000000.0 / elapsed_seconds;
        
        std::cout << "  Elapsed Time: " << duration_ms << " ms" << std::endl;
        std::cout << "  Throughput:   " << std::fixed << std::setprecision(2) << throughput << " events/sec" << std::endl;
        std::cout << "  Total Trades: " << opt_trades.size() << std::endl;
    }

    // Equivalence Check
    std::cout << "\n[VERIFICATION] Verifying trade output matching..." << std::endl;
    bool match = (stl_trades.size() == opt_trades.size());
    if (match) {
        for (size_t i = 0; i < stl_trades.size(); ++i) {
            if (stl_trades[i].buy_id != opt_trades[i].buy_id ||
                stl_trades[i].sell_id != opt_trades[i].sell_id ||
                stl_trades[i].price != opt_trades[i].price ||
                stl_trades[i].qty != opt_trades[i].qty) {
                match = false;
                std::cout << "  Mismatch at Trade #" << i << std::endl;
                break;
            }
        }
    }

    if (match) {
        std::cout << "[VERIFICATION SUCCESS] Both order books produced identical trade files!" << std::endl;
    } else {
        std::cout << "[VERIFICATION FAILED] Trade mismatch detected!" << std::endl;
        std::cout << "  STL trade count: " << stl_trades.size() << " | Opt trade count: " << opt_trades.size() << std::endl;
        return 1;
    }
    std::cout << "==================================================" << std::endl;

    return 0;
}
