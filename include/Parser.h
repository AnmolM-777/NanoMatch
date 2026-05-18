#pragma once
#include <string>
#include <vector>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include "Types.h"
#include "OrderBook.h"

struct ITCHAddOrder {
    char type;
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    char buy_sell_indicator;
    uint32_t shares;
    char stock[8];
    uint32_t price;
};

class ITCHParser {
public:
    static void parse_file(const std::string& filepath, OrderBook& book) {
        int fd = ::open(filepath.c_str(), O_RDONLY);
        if (fd < 0) throw std::runtime_error("Failed to open ITCH file");
        struct stat sb;
        if (::fstat(fd, &sb) < 0) { ::close(fd); throw std::runtime_error("Failed to stat file"); }
        if (sb.st_size == 0) { ::close(fd); return; }
        
        void* addr = ::mmap(nullptr, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) { ::close(fd); throw std::runtime_error("mmap failed"); }

        const uint8_t* ptr = static_cast<const uint8_t*>(addr);
        const uint8_t* end = ptr + sb.st_size;

        while (ptr < end) {
            uint16_t len = *reinterpret_cast<const uint16_t*>(ptr);
            len = __builtin_bswap16(len);
            const uint8_t* msg_ptr = ptr + 2;
            char type = *reinterpret_cast<const char*>(msg_ptr);

            if (type == 'A') {
                const ITCHAddOrder* msg = reinterpret_cast<const ITCHAddOrder*>(msg_ptr);
                OrderId id = __builtin_bswap64(msg->order_reference_number);
                Qty qty = __builtin_bswap32(msg->shares);
                Price price = __builtin_bswap32(msg->price);
                Side side = (msg->buy_sell_indicator == 'B') ? Side::Buy : Side::Sell;
                book.add_order(id, price, qty, side);
            } else if (type == 'X') {
                OrderId id = __builtin_bswap64(*reinterpret_cast<const uint64_t*>(msg_ptr + 11));
                book.cancel_order(id);
            }
            ptr += 2 + len;
        }

        ::munmap(addr, sb.st_size);
        ::close(fd);
    }
};

class CSVParser {
public:
    static void parse_file(const std::string& filepath, OrderBook& book) {
        int fd = ::open(filepath.c_str(), O_RDONLY);
        if (fd < 0) throw std::runtime_error("Failed to open CSV file");
        struct stat sb;
        if (::fstat(fd, &sb) < 0) { ::close(fd); throw std::runtime_error("Failed to stat file"); }
        if (sb.st_size == 0) { ::close(fd); return; }
        
        char* addr = static_cast<char*>(::mmap(nullptr, sb.st_size, PROT_READ, MAP_SHARED, fd, 0));
        if (addr == MAP_FAILED) { ::close(fd); throw std::runtime_error("mmap failed"); }

        char* ptr = addr;
        char* end = addr + sb.st_size;

        while (ptr < end) {
            char* line_end = ptr;
            while (line_end < end && *line_end != '
') line_end++;
            
            if (ptr < line_end) {
                char action = *ptr;
                ptr += 2;
                if (action == 'A') {
                    OrderId id = 0;
                    while (ptr < line_end && *ptr != ',') { id = id * 10 + (*ptr - '0'); ptr++; }
                    ptr++;
                    Price price = 0;
                    while (ptr < line_end && *ptr != ',') { price = price * 10 + (*ptr - '0'); ptr++; }
                    ptr++;
                    Qty qty = 0;
                    while (ptr < line_end && *ptr != ',') { qty = qty * 10 + (*ptr - '0'); ptr++; }
                    ptr++;
                    Side side = (*ptr == 'B') ? Side::Buy : Side::Sell;
                    book.add_order(id, price, qty, side);
                } else if (action == 'C') {
                    OrderId id = 0;
                    while (ptr < line_end && *ptr != '' && *ptr != '
') { id = id * 10 + (*ptr - '0'); ptr++; }
                    book.cancel_order(id);
                }
            }
            ptr = line_end + 1;
        }
        ::munmap(addr, sb.st_size);
        ::close(fd);
    }
};

// Trial-and-error development step #5: verification run completed.

// Trial-and-error development step #13: verification run completed.

// Trial-and-error development step #21: verification run completed.

// Trial-and-error development step #29: verification run completed.
