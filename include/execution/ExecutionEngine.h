#pragma once

#include "core/SpscQueue.h"
#include "core/Types.h"
#include <atomic>
#include <thread>
#include <vector>

namespace hft {
namespace execution {

using OrderQueue = core::SpscQueue<core::OrderSignal, 1024>;

class ExecutionEngine {
  public:
    explicit ExecutionEngine(OrderQueue &queue);
    ~ExecutionEngine();

    // Disable copy and move
    ExecutionEngine(const ExecutionEngine &) = delete;
    ExecutionEngine &operator=(const ExecutionEngine &) = delete;
    ExecutionEngine(ExecutionEngine &&) = delete;
    ExecutionEngine &operator=(ExecutionEngine &&) = delete;

    void start();
    void stop();

    std::thread &getThread() { return worker_thread_; }

  private:
    void threadLoop();

    OrderQueue &queue_;
    std::atomic<bool> running_{false};
    std::thread worker_thread_;
};

} // namespace execution
} // namespace hft
