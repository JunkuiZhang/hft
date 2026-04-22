#include "market/MockMdReceiver.h"
#include <chrono>
#include <cstring>
#include <random>
#include <x86intrin.h>

namespace hft {
namespace market {

MockMdReceiver::MockMdReceiver(TickQueue &queue)
    : queue_(queue), running_(false) {}

MockMdReceiver::~MockMdReceiver() { stop(); }

void MockMdReceiver::start() {
    bool expected = false;
    if (running_.compare_exchange_strong(expected, true)) {
        worker_thread_ = std::thread(&MockMdReceiver::threadLoop, this);
    }
}

void MockMdReceiver::stop() {
    running_.store(false, std::memory_order_release);
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

void MockMdReceiver::threadLoop() {
    std::mt19937 generator(std::random_device{}());
    std::uniform_real_distribution<double> price_dist(3500.0, 3600.0);
    std::uniform_int_distribution<uint32_t> volume_dist(1, 100);

    core::TickData tick;
    // Basic setup for rb2410 contract
    std::strncpy(tick.symbol, "rb2410", sizeof(tick.symbol) - 1);
    tick.symbol[sizeof(tick.symbol) - 1] = 0;
    tick.direction = core::TickDirection::Buy;

    while (running_.load(std::memory_order_acquire)) {
        // Sleep for 500us to simulate CTP market snapshot latency
        std::this_thread::sleep_for(std::chrono::microseconds(500));

        // Generate fake market data
        tick.last_price = price_dist(generator);
        tick.volume = volume_dist(generator);

        // Populate bid/ask levels
        for (int i = 0; i < 5; ++i) {
            tick.bids[i].price = tick.last_price - (i * 0.5);
            tick.bids[i].volume = volume_dist(generator);

            tick.asks[i].price = tick.last_price + (i * 0.5);
            tick.asks[i].volume = volume_dist(generator);
        }

        // T1 Timestamp: Record CPU cycles exactly before pushing.
        tick.local_timestamp = __rdtsc();

        // Push data to the lock-free ring buffer
        // If queue is full, this will spin (or just drop the tick, we drop here
        // to avoid blocking snapshot) Spinning is also valid; we just try once.
        // If queue fails, mock data is discarded (representing buffer overrun).
        if (!queue_.push(tick)) {
            // Yield if full
            std::this_thread::yield();
        }
    }
}

} // namespace market
} // namespace hft
