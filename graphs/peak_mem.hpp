//  peak_memory.hpp  ----------------------------------------------
#pragma once
#include <windows.h>
#include <psapi.h>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <new>
#include <thread>
#include <chrono>
#include <iostream>

// ---------- live-heap tracker -----------------------------------
namespace {
    inline std::atomic_size_t g_cur_heap{0};
    inline std::atomic_size_t g_peak_heap{0};

    inline void add(size_t bytes) noexcept {
        size_t cur = g_cur_heap.fetch_add(bytes, std::memory_order_relaxed) + bytes;
        size_t prev = g_peak_heap.load(std::memory_order_relaxed);
        while (cur > prev &&
               !g_peak_heap.compare_exchange_weak(prev, cur, std::memory_order_relaxed)) {
            /* retry until stored */
        }
    }
    inline void sub(size_t bytes) noexcept {
        g_cur_heap.fetch_sub(bytes, std::memory_order_relaxed);
    }
}

// Reserve space to remember the user-requested size.
void* operator new(std::size_t sz) {
    // +sizeof(size_t) so we can remember the size for delete
    unsigned char* raw = static_cast<unsigned char*>(std::malloc(sz + sizeof(size_t)));
    if (!raw) throw std::bad_alloc();
    *reinterpret_cast<size_t*>(raw) = sz;          // write header
    add(sz);
    return raw + sizeof(size_t);                   // user pointer
}

void operator delete(void* p) noexcept {
    if (!p) return;
    unsigned char* raw = static_cast<unsigned char*>(p) - sizeof(size_t);
    size_t sz = *reinterpret_cast<size_t*>(raw);
    sub(sz);
    std::free(raw);
}

// [] forms forward to the scalar versions
void* operator new[](std::size_t sz)             { return ::operator new(sz); }
void  operator delete[](void* p)    noexcept     { ::operator delete(p); }

// (placement and nothrow overloads left untouched; they don’t allocate)

// ---------- RSS helper (optional polling) -----------------------
class RssPoller {
    HANDLE  proc_ = GetCurrentProcess();
    std::atomic_size_t peak_rss_{0};
    std::thread th_;
    std::atomic_bool stop_{false};
public:
    explicit RssPoller(std::chrono::milliseconds period = std::chrono::milliseconds(10)) {
        th_ = std::thread([this, period] {
            PROCESS_MEMORY_COUNTERS_EX pmc{};
            while (!stop_.load(std::memory_order_relaxed)) {
                if (GetProcessMemoryInfo(proc_,
                    reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                    sizeof(pmc))) {

                    size_t cur = pmc.WorkingSetSize;
                    size_t prev = peak_rss_.load(std::memory_order_relaxed);
                    if (cur > prev)
                        peak_rss_.store(cur, std::memory_order_relaxed);
                }
                std::this_thread::sleep_for(period);
            }
        });
    }
    ~RssPoller() {
        stop_.store(true, std::memory_order_relaxed);
        th_.join();
    }
    size_t peak_bytes() const { return peak_rss_.load(std::memory_order_relaxed); }
};

// ---------- convenience dump ------------------------------------
inline void print_summary(const char* label = "Peak usage") {
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    GetProcessMemoryInfo(GetCurrentProcess(),
        reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc));

    std::cout << "=== " << label << " ===\n"
              << "Peak RSS        : " << pmc.PeakWorkingSetSize / (1024.0 * 1024) << " MiB\n"
              << "Peak live heap  : " << g_peak_heap.load()          / (1024.0 * 1024) << " MiB\n";
}
