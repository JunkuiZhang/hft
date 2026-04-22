#include "strategy/StrategyEngine.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <x86intrin.h>

namespace hft {
namespace strategy {

StrategyEngine::StrategyEngine(TickQueue &queue, OrderQueue &order_queue)
    : queue_(queue), order_queue_(order_queue), running_(false) {
    // Reserve space to avoid reallocations during runtime hot path
    latency_stats_.reserve(100000);

    // Initialize mapping table: "rb2410" -> index 0
    uint64_t key = 0;
    std::memcpy(&key, "rb2410\0\0", 8);
    symbol_idx_map_[key] = 0;
}

StrategyEngine::~StrategyEngine() { stop(); }

void StrategyEngine::start() {
    bool expected = false;
    if (running_.compare_exchange_strong(expected, true)) {
        worker_thread_ = std::thread(&StrategyEngine::threadLoop, this);
    }
}

void StrategyEngine::stop() {
    running_.store(false, std::memory_order_release);
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }

    if (!latency_stats_.empty()) {
        std::sort(latency_stats_.begin(), latency_stats_.end());
        size_t count = latency_stats_.size();
        uint64_t min_lat = latency_stats_.front();
        size_t p99_idx = count * 99 / 100;
        uint64_t p99_lat = latency_stats_[p99_idx];

        uint64_t sum = 0;
        for (auto lat : latency_stats_)
            sum += lat;
        uint64_t avg_lat = sum / count;

        std::cout << "\n--- Strategy Engine Benchmark ---\n"
                  << "Total Ticks: " << count << std::endl
                  << "Min Latency: " << min_lat << " CPU Cycles ("
                  << core::cycles_to_ns(min_lat) << " ns)\n"
                  << "Avg Latency: " << avg_lat << " CPU Cycles ("
                  << core::cycles_to_ns(avg_lat) << " ns)\n"
                  << "P99 Latency: " << p99_lat << " CPU Cycles ("
                  << core::cycles_to_ns(p99_lat) << " ns)\n"
                  << "-----------------------------------\n";

        latency_stats_.clear();
    }
}

void StrategyEngine::threadLoop() {
    core::TickData tick;

    // Busy polling loop - no sleep/yield to ensure lowest latency
    while (running_.load(std::memory_order_acquire)) {
        if (queue_.pop(tick)) [[likely]] {
            // T2 Timestamp: calculate CPU cycles taken for penetration
            uint64_t t2 = core::rdtsc_end();

            // Calculate Latency (Delta Cycles)
            uint64_t delta_cycles = t2 - tick.local_timestamp;
            latency_stats_.push_back(delta_cycles);

            // Fast mapping O(1) without strncmp
            uint64_t key;
            std::memcpy(&key, tick.symbol, 8);

            auto it = symbol_idx_map_.find(key);
            if (it != symbol_idx_map_.end()) [[likely]] {
                flat_order_book_[it->second] =
                    tick; // Update O(1) using static array
            }

            // Compute OBI (Order Book Imbalance) based on level 1 bid and ask
            // volume
            double obi [[maybe_unused]] = 0.0;
            uint32_t bid1_vol = tick.bids[0].volume;
            uint32_t ask1_vol = tick.asks[0].volume;
            uint32_t total_vol = bid1_vol + ask1_vol;

            if (total_vol > 0) {
                // Ensure floating point div avoids weird issues, compute
                // correctly
                obi = (static_cast<double>(bid1_vol) -
                       static_cast<double>(ask1_vol)) /
                      static_cast<double>(total_vol);

                // Trigger an order if imbalance is strongly on ask side and we
                // have sufficient stats collected
                if (obi > 0.8 && latency_stats_.size() >= 1000) [[unlikely]] {
                    core::OrderSignal signal;
                    std::memcpy(signal.symbol, tick.symbol,
                                sizeof(signal.symbol));
                    signal.price =
                        tick.bids[0].price; // Take the bid 1 price if we are
                                            // selling due to bid strength
                    signal.volume = 1;
                    signal.action = core::OrderAction::Buy;
                    signal.tick_timestamp =
                        tick.local_timestamp; // T1 (Arrival)
                    signal.signal_timestamp =
                        core::rdtsc_start(); // T3 (Signal Generation)

                    if (!order_queue_.push(signal)) {
                        // Drop signal if queue full
                    }
                }
            }
        }
    }
}

} // namespace strategy
} // namespace hft
