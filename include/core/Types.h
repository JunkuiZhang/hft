#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <x86intrin.h>

namespace hft {
namespace core {

// Price/Volume types for 5 levels (Bid/Ask)
struct MarketLevel {
    double price;
    uint32_t volume;
};

// TickData aligned to a 64-byte boundary to avoid false sharing
// in concurrent disruptors / ringbuffers.
enum class TickDirection : uint8_t { Unknown = 0, Buy = 1, Sell = 2 };

struct alignas(64) TickData {
    char symbol[16];          // 16 bytes: contract name
    uint64_t local_timestamp; // 8 bytes: arrival timestamp (ns), T1
    double last_price;        // 8 bytes: latest traded price

    std::array<MarketLevel, 5>
        bids; // 5 * 12 = 60 bytes (actually 5 * 16 = 80 bytes due to padding)
    std::array<MarketLevel, 5> asks; // 5 * 12 = 60 bytes (actually 80 bytes)

    uint32_t volume;         // 4 bytes: total volume
    TickDirection direction; // 1 byte: trade direction

    // Total size should naturally be a multiple of 64 bytes due to alignas(64)
};

// -------------------------------------------------------------

enum class OrderAction : uint8_t { Buy = 0, Sell = 1 };

// OrderSignal aligned to a 64-byte boundary.
struct alignas(64) OrderSignal {
    char symbol[16];           // 16 bytes: contract name
    uint64_t tick_timestamp;   // 8 bytes: tick arrival time (T1)
    uint64_t signal_timestamp; // 8 bytes: signal generation time (T3)
    double price;              // 8 bytes: target price
    uint32_t volume;           // 4 bytes: target volume
    OrderAction action;        // 1 byte: Buy/Sell

    // Total size padded to 64 bytes
};

// 获取当前 CPU 的 TSC (Time Stamp Counter) 频率
// 注意：实盘中通常会在系统启动时通过 sleep(1) 测量一次 TSC
// 的增加量来标定这个常数 假设你的 CPU 基础频率是 3.0 GHz (3,000,000,000 Hz)
// 那么 1 个 cycle = 1 / 3.0 ns = 0.333 ns
inline double NS_PER_CYCLE = 0.333; // 将通过 calibrate_tsc() 标定

// 适合放在动作【之前】的计时
inline uint64_t rdtsc_start() {
    _mm_lfence(); // 阻止后续指令被乱序提前执行
    return __rdtsc();
}

// 适合放在动作【之后】的计时
inline uint64_t rdtsc_end() {
    unsigned int aux;
    uint64_t tsc = __rdtscp(&aux); // 等待前面的指令全部退役(Retire)后再读
    _mm_lfence();                  // 阻止后续指令乱序跑到前面去
    return tsc;
}

// 将 Cycles 转换为纳秒
inline uint64_t cycles_to_ns(uint64_t cycles) {
    return static_cast<uint64_t>(cycles * NS_PER_CYCLE);
}

// 运行时标定 TSC 频率
inline void calibrate_tsc() {
    std::cout << "Calibrating TSC frequency... (Wait 1 second)" << std::endl;
    uint64_t start = rdtsc_start();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    uint64_t end = rdtsc_end();
    uint64_t cycles = end - start;
    NS_PER_CYCLE = 1'000'000'000.0 / static_cast<double>(cycles);
    std::cout << "TSC Calibration complete. CPU Frequency: "
              << (cycles / 1'000'000'000.0) << " GHz" << std::endl;
}

} // namespace core
} // namespace hft
