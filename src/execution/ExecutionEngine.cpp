#include "execution/ExecutionEngine.h"
#include <iostream>
#include <x86intrin.h>

namespace hft {
namespace execution {

ExecutionEngine::ExecutionEngine(OrderQueue &queue)
    : queue_(queue), running_(false) {}

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
}

void ExecutionEngine::threadLoop() {
    core::OrderSignal order;

    // Busy polling loop - no sleep/yield
    while (running_.load(std::memory_order_acquire)) {
        if (queue_.pop(order)) [[likely]] {
            uint64_t t3 = __rdtsc();

            // Calculate latency from Strategy generation to Execution arrival
            uint64_t delta_cycles = t3 - order.timestamp;

            std::cout << "[Execution] Action: "
                      << (order.action == core::OrderAction::Buy ? "Buy "
                                                                 : "Sell")
                      << " | Sym: " << order.symbol
                      << " | Price: " << order.price
                      << " | Vol: " << order.volume
                      << " | Delta: " << delta_cycles << " Cycles\n";
        }
    }
}

} // namespace execution
} // namespace hft
