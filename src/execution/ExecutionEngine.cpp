#include "execution/ExecutionEngine.h"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <x86intrin.h>

namespace hft {
namespace execution {

ExecutionEngine::ExecutionEngine(OrderQueue &queue)
    : queue_(queue), running_(false) {
    latency_stats_.reserve(
        1000000); // Pre-allocate to prevent dynamic alloc in hot path
}

ExecutionEngine::~ExecutionEngine() { stop(); }

void ExecutionEngine::start() {
    bool expected = false;
    if (running_.compare_exchange_strong(expected, true)) {
        worker_thread_ = std::thread(&ExecutionEngine::threadLoop, this);
    }
}

void ExecutionEngine::stop() {
    running_.store(false, std::memory_order_release);
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }

    if (!latency_stats_.empty()) {
        std::sort(latency_stats_.begin(), latency_stats_.end());
        size_t count = latency_stats_.size();

        uint64_t sum =
            std::accumulate(latency_stats_.begin(), latency_stats_.end(), 0ULL);
        uint64_t avg = sum / count;
        uint64_t p99 = latency_stats_[count * 99 / 100];
        uint64_t p999 = latency_stats_[count * 999 / 1000];

        // Calculate max jitter (max consecutive difference)
        uint64_t max_jitter = 0;
        for (size_t i = 1; i < count; ++i) {
            uint64_t diff = latency_stats_[i] - latency_stats_[i - 1];
            if (diff > max_jitter)
                max_jitter = diff;
        }

        std::cout << "\n--- Execution Engine Downlink Benchmark ---\n"
                  << "Total Orders: " << count << "\n"
                  << "Avg Latency (T4 - T1): " << avg << " Cycles ("
                  << core::cycles_to_ns(avg) << " ns)\n"
                  << "P99 Latency: " << p99 << " Cycles ("
                  << core::cycles_to_ns(p99) << " ns)\n"
                  << "P99.9 Latency: " << p999 << " Cycles ("
                  << core::cycles_to_ns(p999) << " ns)\n"
                  << "Max Jitter: " << max_jitter << " Cycles ("
                  << core::cycles_to_ns(max_jitter) << " ns)\n"
                  << "-------------------------------------------\n";

        latency_stats_.clear();
    }
}

void ExecutionEngine::threadLoop() {
    core::OrderSignal order;

    // Busy polling loop - no sleep/yield
    while (running_.load(std::memory_order_acquire)) {
        if (queue_.pop(order)) [[likely]] {
            uint64_t t4 = core::rdtsc_end();

            // Calculate overall latency from Tick arrival to Downlink execution
            uint64_t delta_cycles = t4 - order.tick_timestamp;
            latency_stats_.push_back(delta_cycles);
        }
    }
}

} // namespace execution
} // namespace hft
