/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <ki6080@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.      Seungwoo Kang.
 * ----------------------------------------------------------------------------
 */
#pragma once
#include <atomic>
#include <thread>

namespace kangsw:: inline threads {
//! @see https://rigtorp.se/spinlock/
//! Applied slight modification to use atomic_flag
struct spinlock {
    std::atomic_flag lock_;

    void lock() noexcept {
        for (;;) {
            // Optimistically assume the lock is free on the first try
            if (!lock_.test_and_set(std::memory_order_acquire)) {
                return;
            }
            // Wait for lock to be released without generating cache misses
            while (lock_.test(std::memory_order_relaxed)) {
                // Issue X86 PAUSE or ARM YIELD instruction to reduce contention between
                // hyper-threads
                std::this_thread::yield();
            }
        }
    }

    bool try_lock() noexcept {
        // First do a relaxed load to check if lock is free in order to prevent
        // unnecessary cache misses if someone does while(!try_lock())
        return !lock_.test(std::memory_order_relaxed) && !lock_.test_and_set(std::memory_order_acquire);
    }

    void unlock() noexcept {
        lock_.clear(std::memory_order_release);
    }
};
} // namespace kangsw