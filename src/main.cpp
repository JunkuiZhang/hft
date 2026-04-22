#include "core/SpscQueue.h"
#include "core/Types.h"
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

  // Small SPSC Test
  TickQueue queue;
  std::thread producer([&queue]() {
    hft::core::TickData tick{};
    tick.last_price = 100.5;
    tick.volume = 10;
    while (!queue.push(tick)) {
      std::this_thread::yield();
    }
  });

  std::thread consumer([&queue]() {
    hft::core::TickData tick;
    while (!queue.pop(tick)) {
      std::this_thread::yield();
    }
    std::cout << "\\n[Consumer] Received tick with price: " << tick.last_price
              << ", volume: " << tick.volume << std::endl;
  });

  producer.join();
  consumer.join();

  return 0;
}
