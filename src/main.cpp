#include "core/SpscQueue.h"
#include "core/Types.h"
#include "execution/ExecutionEngine.h"
#include "market/MockMdReceiver.h"
#include "strategy/StrategyEngine.h"
#include <chrono>
#include <iostream>
#include <pthread.h> // For thread affinity
#include <thread>

void pin_thread_to_core(std::thread &t, int core_id) {
    if (!t.joinable())
        return;

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    int rc =
        pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    } else {
        std::cout << "Successfully pinned thread to core " << core_id << "\n";
    }
}

int main() {
    std::cout << "--- HFT Engine (C++20) ---" << std::endl;
    std::cout << "TickData Memory Layout:" << std::endl;
    std::cout << "  sizeof(TickData):  " << sizeof(hft::core::TickData)
              << " bytes" << std::endl;
    std::cout << "  alignof(TickData): " << alignof(hft::core::TickData)
              << " bytes" << std::endl;

    std::cout << "\nOrderSignal Memory Layout:" << std::endl;
    std::cout << "  sizeof(OrderSignal):  " << sizeof(hft::core::OrderSignal)
              << " bytes" << std::endl;
    std::cout << "  alignof(OrderSignal): " << alignof(hft::core::OrderSignal)
              << " bytes" << std::endl;

    std::cout << "\nSpscQueue Layout:" << std::endl;
    using TickQueue = hft::core::SpscQueue<hft::core::TickData, 1024>;
    std::cout << "  sizeof(TickQueue): " << sizeof(TickQueue) << " bytes"
              << std::endl;
    std::cout << "  alignof(TickQueue): " << alignof(TickQueue) << " bytes"
              << std::endl;

    std::cout << "\\nStarting Complete HFT Pipeline Test..." << std::endl;
    using OrderQueue = hft::core::SpscQueue<hft::core::OrderSignal, 1024>;

    TickQueue tick_queue;
    OrderQueue order_queue;

    hft::execution::ExecutionEngine execution(order_queue);
    hft::market::MockMdReceiver receiver(tick_queue);
    hft::strategy::StrategyEngine strategy(tick_queue, order_queue);

    // Start execution thread on core 4
    execution.start();
    pin_thread_to_core(execution.getThread(), 4);

    // Start strategy thread (busy polling)
    strategy.start();
    // Pin strategy thread to core 3
    pin_thread_to_core(strategy.getThread(), 3);

    // Start generating market data
    receiver.start();
    // Pin market receiver thread to core 2
    pin_thread_to_core(receiver.getThread(), 2);

    // Run simulation for exactly 1500 milliseconds (to accumulate >1000 ticks)
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    receiver.stop();
    strategy.stop();
    execution.stop();

    std::cout << "Done!" << std::endl;

    return 0;
}
