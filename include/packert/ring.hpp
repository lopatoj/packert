// Copyright 2026 Justin Lopato.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#ifndef PACKERT__RING_HPP_
#define PACKERT__RING_HPP_

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace packert {

class RingBuffer {
public:
  static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

  RingBuffer() = delete;

  RingBuffer(std::size_t capacity) : capacity_(capacity) {
    buffer_ = new std::uint8_t[capacity_];
  }

  RingBuffer(std::uint8_t *buffer, std::size_t capacity)
      : buffer_(buffer), capacity_(capacity) {}

  RingBuffer(const RingBuffer &) = delete;

  ~RingBuffer() = default;

  std::size_t capacity() noexcept { return capacity_; }

  std::size_t size() const noexcept { return size_; }

  bool empty() const noexcept { return size_ == 0; }

  bool full() const noexcept { return size_ == capacity_; }

  void clear() noexcept {
    start_ = 0;
    end_ = 0;
    size_ = 0;
  }

  std::size_t push(const std::uint8_t *data, std::size_t size) noexcept {
    std::size_t written = 0;
    while (written < size && size_ < capacity_) {
      buffer_[end_] = data[written];
      end_ = (end_ + 1) % capacity_;
      ++size_;
      ++written;
    }
    return written;
  }

  std::span<std::byte> pop() noexcept {
    std::uint8_t *data;
    std::size_t size;
    std::size_t read = 0;
    while (read < size && size_ > 0) {
      data[read] = buffer_[start_];
      start_ = (start_ + 1) % capacity_;
      --size_;
      ++read;
    }
    return std::span<std::byte>(reinterpret_cast<std::byte *>(data), read);
  }

  std::span<std::byte> peek(std::uint8_t *data,
                            std::size_t size) const noexcept {
    const std::size_t available = (size < size_) ? size : size_;
    for (std::size_t i = 0; i < available; ++i) {
      data[i] = buffer_[(start_ + i) % capacity_];
    }
    return std::span<std::byte>(reinterpret_cast<std::byte *>(data), available);
  }

  std::size_t skip(std::size_t count) noexcept {
    const std::size_t dropped = (count < size_) ? count : size_;
    start_ = (start_ + dropped) % capacity_;
    size_ -= dropped;
    return dropped;
  }

  std::size_t find(std::uint8_t value) const noexcept {
    for (std::size_t i = 0; i < size_; ++i) {
      if (buffer_[(start_ + i) % capacity_] == value) {
        return i;
      }
    }
    return npos;
  }

  std::uint8_t at(std::size_t offset) const noexcept {
    return buffer_[(start_ + offset) % capacity_];
  }

private:
  std::uint8_t *buffer_ = nullptr;
  std::size_t capacity_ = 0;
  std::size_t start_ = 0;
  std::size_t end_ = 0;
  std::size_t size_ = 0;
};

} // namespace packert

#endif // PACKERT__RING_HPP_
