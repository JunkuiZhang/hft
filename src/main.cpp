#include "core/SpscQueue.h"
#include "core/Types.h"
#include "market/MockMdReceiver.h"
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

  std::cout << "\\nStarting Mock Market Data Receiver Test..." << std::endl;
  TickQueue queue;

  hft::market::MockMdReceiver receiver(queue);

  // Start generating market data
  receiver.start();

  // Consumer thread
  std::thread consumer([&queue]() {
    hft::core::TickData tick;
    int received_count = 0;
    while (received_count < 10) { // Wait for 10 ticks
      if (queue.pop(tick)) {
        auto now_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch())
                .count();

        // Compute penetration latency
        uint64_t latency = now_ns - tick.local_timestamp;

        std::cout << "[Consumer] Symbol: " << tick.symbol
                  << " | Price: " << tick.last_price
                  << " | T1: " << tick.local_timestamp
                  << " | Latency: " << latency << " ns\\n";

        received_count++;
      } else {
        std::this_thread::yield();
      }
    }
  });

  consumer.join();
  receiver.stop();

  std::cout << "Done!" << std::endl;

  return 0;
}
