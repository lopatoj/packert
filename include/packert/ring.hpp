// Copyright 2026 Justin Lopato.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#ifndef PACKERT__RING_HPP_
#define PACKERT__RING_HPP_

#include <cstddef>
#include <span>

namespace packert {

class ring_buffer {
 public:
  ring_buffer() = delete;

  ring_buffer(const ring_buffer &) = delete;

  ring_buffer(std::span<std::byte> buffer) noexcept : buffer_(buffer) {}

  ~ring_buffer() = default;

  std::size_t capacity() noexcept { return buffer_.size(); }

  std::size_t size() const noexcept { return size_; }

  bool empty() const noexcept { return size_ == 0; }

  bool full() const noexcept { return size_ == buffer_.size(); }

  void clear() noexcept {
    start_ = 0;
    end_ = 0;
    size_ = 0;
  }

  std::size_t push(std::span<const std::byte> buffer) noexcept {
    std::size_t written = 0;
    while (written < buffer.size() && size_ < buffer_.size()) {
      buffer_[end_] = buffer[written];
      end_ = (end_ + 1) % buffer_.size();
      ++size_;
      ++written;
    }
    return written;
  }

  std::span<std::byte> pop(std::span<std::byte> buffer) noexcept {
    std::size_t read = 0;
    while (read < buffer.size() && size_ > 0) {
      buffer[read] = buffer_[start_];
      start_ = (start_ + 1) % buffer_.size();
      --size_;
      ++read;
    }
    return {buffer.data(), read};
  }

  std::span<std::byte> peek(std::span<std::byte> buffer) const noexcept {
    const std::size_t available =
        (buffer.size() < size_) ? buffer.size() : size_;
    for (std::size_t i = 0; i < available; ++i) {
      buffer[i] = buffer_[(start_ + i) % buffer_.size()];
    }
    return {buffer.data(), available};
  }

  std::size_t skip(std::size_t count) noexcept {
    const std::size_t dropped = (count < size_) ? count : size_;
    start_ = (start_ + dropped) % buffer_.size();
    size_ -= dropped;
    return dropped;
  }

  std::byte *find(std::byte value) const noexcept {
    for (std::size_t i = 0; i < size_; ++i) {
      if (buffer_[(start_ + i) % buffer_.size()] == value) {
        return &buffer_[(start_ + i) % buffer_.size()];
      }
    }
    return nullptr;
  }

  std::byte at(std::size_t offset) const noexcept {
    return buffer_[(start_ + offset) % buffer_.size()];
  }

 private:
  std::span<std::byte> buffer_;
  std::size_t start_ = 0;
  std::size_t end_ = 0;
  std::size_t size_ = 0;
};

}  // namespace packert

#endif  // PACKERT__RING_HPP_
