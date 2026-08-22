// Copyright 2026 Justin Lopato.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#ifndef PACKERT__TRANSPORT_HPP_
#define PACKERT__TRANSPORT_HPP_

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <system_error>

namespace packert {

/**
 * @brief Hardware abstraction layer for an arbitrary communication interface.
 */
class Transport {
 public:
  class managed_buffer {
   public:
    managed_buffer(Transport &transport, std::span<const std::byte> buffer,
                   std::uint16_t id)
        : transport_(transport), buffer_(buffer), id_(id) {}

    // Copy and move operations are deleted to prevent misuse.
    managed_buffer(const managed_buffer &) = delete;
    managed_buffer &operator=(const managed_buffer &) = delete;

    managed_buffer(managed_buffer &&other) noexcept
        : transport_(other.transport_), buffer_(other.buffer_), id_(other.id_) {
      other.released_ = true;
    }

    ~managed_buffer() {
      if (!released_) {
        transport_.relinquish(*this);
      }
    }

    std::size_t size() const noexcept { return buffer_.size(); }
    const std::byte *data() const noexcept { return buffer_.data(); }
    std::span<const std::byte> span() const noexcept { return buffer_; }
    std::uint16_t id() const noexcept { return id_; }

   private:
    Transport &transport_;
    std::span<const std::byte> buffer_;
    std::uint16_t id_;
    bool released_ = false;
  };

  virtual ~Transport() = default;

  /**
   * @brief Writes packet to transport.
   * @param buffer The packet to send.
   * @return std::error_code indicating success or failure.
   */
  virtual std::error_code send(std::span<const std::byte> buffer) = 0;

  /**
   * @brief Loads packet into provided buffer if a new packet is available.
   * @return std::expected<managed_buffer, std::error_code>
   * indicating success or failure. Returns errc::no_message_available if no
   * packet is available.
   */
  virtual std::expected<managed_buffer, std::error_code> receive() = 0;

  /**
   * @brief Relinquishes ownership of a buffer provided by a call to receive
   * back to the transport.
   * @param buffer The buffer to relinquish.
   */
  virtual void relinquish(managed_buffer &buffer) = 0;
};

}  // namespace packert

#endif  // PACKERT__TRANSPORT_HPP_
