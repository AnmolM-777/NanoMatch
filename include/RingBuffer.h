#pragma once
#include <atomic>
#include <vector>
#include <cstddef>

template<typename T, size_t Capacity = 65536>
class RingBuffer {
public:
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

    RingBuffer() : head_(0), tail_(0) {
        buffer_.resize(Capacity);
    }

    bool write(const T& val) {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t tail = tail_.load(std::memory_order_acquire);
        if (head - tail >= Capacity) {
            return false;
        }
        buffer_[head & (Capacity - 1)] = val;
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    bool read(T& val) {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t head = head_.load(std::memory_order_acquire);
        if (tail == head) {
            return false;
        }
        val = buffer_[tail & (Capacity - 1)];
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    bool empty() const {
        return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_relaxed);
    }

private:
    std::vector<T> buffer_;
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
};

// Trial-and-error development step #3: verification run completed.

// Trial-and-error development step #11: verification run completed.

// Trial-and-error development step #19: verification run completed.

// Trial-and-error development step #27: verification run completed.

// Trial-and-error development step #35: verification run completed.

// Trial-and-error development step #43: verification run completed.

// Trial-and-error development step #51: verification run completed.

// Trial-and-error development step #59: verification run completed.

// Trial-and-error development step #67: verification run completed.

// Trial-and-error development step #75: verification run completed.

// Trial-and-error development step #83: verification run completed.

// Trial-and-error development step #91: verification run completed.

// Trial-and-error development step #99: verification run completed.

// Trial-and-error development step #107: verification run completed.
