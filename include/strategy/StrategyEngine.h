#pragma once

#include "core/SpscQueue.h"
#include "core/Types.h"
#include <array>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <vector>

namespace hft {
namespace strategy {

using TickQueue = core::SpscQueue<core::TickData, 1024>;
using OrderQueue = core::SpscQueue<core::OrderSignal, 1024>;

class StrategyEngine {
  public:
    explicit StrategyEngine(TickQueue &queue, OrderQueue &order_queue);
    ~StrategyEngine();

    // Disable copy and move
    StrategyEngine(const StrategyEngine &) = delete;
    StrategyEngine &operator=(const StrategyEngine &) = delete;
    StrategyEngine(StrategyEngine &&) = delete;
    StrategyEngine &operator=(StrategyEngine &&) = delete;

    // Start the strategy polling thread
    void start();

    // Stop the strategy polling thread
    void stop();

    // Get the worker thread mapped to cores
    std::thread &getThread() { return worker_thread_; }

  private:
    void threadLoop();

    TickQueue &queue_;
    OrderQueue &order_queue_;
    std::atomic<bool> running_{false};
    std::thread worker_thread_;

    // Static array memory cache for order books to prevent cache misses O(1)
    std::array<core::TickData, 256> flat_order_book_;

    // Mapping table: maps 8-byte symbol string to 0-255 index
    std::unordered_map<uint64_t, uint8_t> symbol_idx_map_;

    // Latency statistics (CPU Cycles)
    std::vector<uint64_t> latency_stats_;
};

} // namespace strategy
} // namespace hft
