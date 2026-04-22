#pragma once

#include "core/SpscQueue.h"
#include "core/Types.h"
#include <atomic>
#include <thread>

namespace hft {
namespace market {

// Hardcode queue capacity to 1024 for simplicity in this abstract mock
using TickQueue = core::SpscQueue<core::TickData, 1024>;

class MockMdReceiver {
  public:
    explicit MockMdReceiver(TickQueue &queue);
    ~MockMdReceiver();

    // Disable copy and move
    MockMdReceiver(const MockMdReceiver &) = delete;
    MockMdReceiver &operator=(const MockMdReceiver &) = delete;
    MockMdReceiver(MockMdReceiver &&) = delete;
    MockMdReceiver &operator=(MockMdReceiver &&) = delete;

    // Start the mock receiver thread
    void start();

    // Stop the mock receiver thread
    void stop();

    // Get the worker thread mapped to cores
    std::thread &getThread() { return worker_thread_; }

  private:
    void threadLoop();
    void generateTick(core::TickData &tick);

    TickQueue &queue_;
    std::atomic<bool> running_{false};
    std::thread worker_thread_;
};

} // namespace market
} // namespace hft
