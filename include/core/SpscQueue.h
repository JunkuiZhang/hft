#pragma once

#include <atomic>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace hft {
namespace core {

// SpscQueue (Single-Producer Single-Consumer) Lock-free Ring Buffer
// Capacity MUST be a power of 2 for fast modulo operations.
template <typename T, size_t Capacity> class SpscQueue {
  static_assert(Capacity > 0, "Capacity must be greater than 0");
  static_assert((Capacity & (Capacity - 1)) == 0,
                "Capacity must be a power of 2");

public:
  SpscQueue()
      : write_index_(0), cached_read_index_(0), read_index_(0),
        cached_write_index_(0) {}

  // Disable copy and assignment
  SpscQueue(const SpscQueue &) = delete;
  SpscQueue &operator=(const SpscQueue &) = delete;

  // Push by const reference
  bool push(const T &item) {
    const size_t current_write = write_index_.load(std::memory_order_relaxed);

    // Use cached read index to avoid frequent cache line invalidation
    if (current_write - cached_read_index_ == Capacity) {
      cached_read_index_ = read_index_.load(std::memory_order_acquire);
      if (current_write - cached_read_index_ == Capacity) {
        return false; // Queue is full
      }
    }

    data_[current_write & Mask] = item;
    write_index_.store(current_write + 1, std::memory_order_release);
    return true;
  }

  // Push by rvalue reference (move)
  bool push(T &&item) {
    const size_t current_write = write_index_.load(std::memory_order_relaxed);

    if (current_write - cached_read_index_ == Capacity) {
      cached_read_index_ = read_index_.load(std::memory_order_acquire);
      if (current_write - cached_read_index_ == Capacity) {
        return false; // Queue is full
      }
    }

    data_[current_write & Mask] = std::move(item);
    write_index_.store(current_write + 1, std::memory_order_release);
    return true;
  }

  // Pop element into target
  bool pop(T &item) {
    const size_t current_read = read_index_.load(std::memory_order_relaxed);

    if (cached_write_index_ == current_read) {
      cached_write_index_ = write_index_.load(std::memory_order_acquire);
      if (cached_write_index_ == current_read) {
        return false; // Queue is empty
      }
    }

    item = std::move(data_[current_read & Mask]);
    read_index_.store(current_read + 1, std::memory_order_release);
    return true;
  }

  size_t size() const {
    // Approximate size due to concurrent modifications
    size_t w = write_index_.load(std::memory_order_acquire);
    size_t r = read_index_.load(std::memory_order_acquire);
    return w - r;
  }

  bool empty() const { return size() == 0; }

private:
  static constexpr size_t Mask = Capacity - 1;

  // Producer cache line
  alignas(64) std::atomic<size_t> write_index_;
  size_t cached_read_index_;

  // Consumer cache line
  alignas(64) std::atomic<size_t> read_index_;
  size_t cached_write_index_;

  // Data cache line
  alignas(64) T data_[Capacity];
};

} // namespace core
} // namespace hft
