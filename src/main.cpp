#include "core/SpscQueue.h"
#include "core/Types.h"
#include "market/MockMdReceiver.h"
#include "strategy/StrategyEngine.h"
#include <chrono>
#include <cstddef>
#include <iostream>
#include <thread>

int main() {
    std::cout << "--- HFT Engine (C++20) ---" << std::endl;
    std::cout << "TickData Memory Layout:" << std::endl;
    std::cout << "  sizeof(TickData):  " << sizeof(hft::core::TickData)
              << " bytes" << std::endl;
    std::cout << "  alignof(TickData): " << alignof(hft::core::TickData)
              << " bytes" << std::endl;

    std::cout << "\\nOrderSignal Memory Layout:" << std::endl;
    std::cout << "  sizeof(OrderSignal):  " << sizeof(hft::core::OrderSignal)
              << " bytes" << std::endl;
    std::cout << "  alignof(OrderSignal): " << alignof(hft::core::OrderSignal)
              << " bytes" << std::endl;

    std::cout << "\\nSpscQueue Layout:" << std::endl;
    using TickQueue = hft::core::SpscQueue<hft::core::TickData, 1024>;
    std::cout << "  sizeof(TickQueue): " << sizeof(TickQueue) << " bytes"
              << std::endl;
    std::cout << "  alignof(TickQueue): " << alignof(TickQueue) << " bytes"
              << std::endl;

    std::cout << "\\nStarting Complete HFT Pipeline Test..." << std::endl;
    TickQueue queue;

    hft::market::MockMdReceiver receiver(queue);
    hft::strategy::StrategyEngine strategy(queue);

    // Start strategy thread (busy polling)
    strategy.start();

    // Start generating market data
    receiver.start();

    // Run simulation for exactly 10 milliseconds
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    receiver.stop();
    strategy.stop();

    std::cout << "Done!" << std::endl;

    return 0;
}
