#include "strategy/StrategyEngine.h"
#include <chrono>
#include <cstring>
#include <iostream>

namespace hft {
namespace strategy {

StrategyEngine::StrategyEngine(TickQueue &queue)
    : queue_(queue), running_(false) {
    // Reserve space to avoid reallocations during runtime
    flat_order_book_.reserve(128);
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
}

void StrategyEngine::updateOrderBook(const core::TickData &tick) {
    // Linear scan for symbol in flat_order_book
    // For small sets of instruments, linear array scan is faster than map
    // lookups due to contiguous memory
    for (auto &entry : flat_order_book_) {
        if (std::strncmp(entry.symbol, tick.symbol,
                         sizeof(core::TickData::symbol)) == 0) {
            entry = tick; // Update latest snapshot
            return;
        }
    }

    // Symbol not found, append it
    flat_order_book_.push_back(tick);
}

void StrategyEngine::threadLoop() {
    core::TickData tick;

    // Busy polling loop - no sleep/yield to ensure lowest latency
    while (running_.load(std::memory_order_acquire)) {
        if (queue_.pop(tick)) [[likely]] {
            // T2 Timestamp: immediately after popping the value from the queue
            auto now = std::chrono::high_resolution_clock::now();
            uint64_t t2 = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              now.time_since_epoch())
                              .count();

            // Calculate Latency (Delta T)
            uint64_t delta_t = t2 - tick.local_timestamp;

            // Update flat order book cache
            updateOrderBook(tick);

            // Compute OBI (Order Book Imbalance) based on level 1 bid and ask
            // volume
            double obi = 0.0;
            uint32_t bid1_vol = tick.bids[0].volume;
            uint32_t ask1_vol = tick.asks[0].volume;
            uint32_t total_vol = bid1_vol + ask1_vol;

            if (total_vol > 0) {
                // Ensure floating point div avoids weird issues, compute
                // correctly
                obi = (static_cast<double>(bid1_vol) -
                       static_cast<double>(ask1_vol)) /
                      static_cast<double>(total_vol);
            }

            // Print info
            std::cout << "[Strategy] Sym: " << tick.symbol
                      << " | DeltaT: " << delta_t << " ns"
                      << " | OBI: " << obi << " | Bid1V: " << bid1_vol
                      << " Ask1V: " << ask1_vol << "\n";
        }
    }
}

} // namespace strategy
} // namespace hft
