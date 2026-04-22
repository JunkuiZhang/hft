#pragma once

#include <array>
#include <cstdint>

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
    char symbol[16];    // 16 bytes: contract name
    uint64_t timestamp; // 8 bytes: signal generation time
    double price;       // 8 bytes: target price
    uint32_t volume;    // 4 bytes: target volume
    OrderAction action; // 1 byte: Buy/Sell

    // Total size padded to 64 bytes
};

} // namespace core
} // namespace hft
