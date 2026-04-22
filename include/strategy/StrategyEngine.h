#pragma once

#include "core/SpscQueue.h"
#include "core/Types.h"
#include <atomic>
#include <thread>
#include <vector>

namespace hft {
namespace strategy {

using TickQueue = core::SpscQueue<core::TickData, 1024>;

class StrategyEngine {
  public:
    explicit StrategyEngine(TickQueue &queue);
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

    // Flat order book implementation to ensure L1 Cache performance
    // Avoids map/unordered_map. Uses simple linear search for a small number of
    // symbols or direct indexing if symbols are mapped to integers previously.
    void updateOrderBook(const core::TickData &tick);

    TickQueue &queue_;
    std::atomic<bool> running_{false};
    std::thread worker_thread_;

    // Flattened contiguous memory cache for order books to prevent cache misses
    std::vector<core::TickData> flat_order_book_;
};

} // namespace strategy
} // namespace hft
