// Copyright 2026 Justin Lopato.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#ifndef PACKERT__PACKET_HPP_
#define PACKERT__PACKET_HPP_

#include "packert/crc.hpp"
#include "packert/message.hpp"
#include "packert/visibility_control.h"
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <system_error>

namespace packert {

namespace {

struct [[gnu::packed]] PACKERT_PUBLIC Frame {
  std::uint8_t sync = 0x00;
  std::uint32_t crc32;
  std::uint64_t timestamp;
  std::uint16_t sequence;
  MessageId message_id;
  std::size_t message_size;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
  std::byte message[];
#pragma GCC diagnostic pop
};

} // namespace

template <typename M> class PACKERT_PUBLIC Packet {
public:
  Packet() {}
  Packet(M message) : message_(message) {
    MessageTraits<M>::encode(message, message_buffer_, message_size_);

    timestamp_ = 0; // TODO: Set timestamp
    sequence_ = 0;  // TODO: Set sequence
    message_id_ = MessageTraits<M>::ID;
    crc32_ = Crc32::calculate(
        std::span<const std::byte>(message_buffer_.data(), message_size_));
  };

  std::error_code load(std::span<const std::byte> buffer) {
    if (buffer.size() < sizeof(Frame) + MessageTraits<M>::SIZE) {
      return std::make_error_code(std::errc::invalid_argument);
    }

    const Frame *frame = reinterpret_cast<const Frame *>(buffer.data());

    if (frame->message_id != MessageTraits<M>::ID ||
        frame->message_size != MessageTraits<M>::SIZE) {
      return std::make_error_code(std::errc::invalid_argument);
    }

    const std::span<const std::byte> message_buffer(
        reinterpret_cast<const std::byte *>(frame->message),
        frame->message_size);

    if (Crc32::calculate(message_buffer) != frame->crc32) {
      return std::make_error_code(std::errc::invalid_argument);
    }

    crc32_ = frame->crc32;
    timestamp_ = frame->timestamp;
    sequence_ = frame->sequence;
    message_id_ = frame->message_id;

    return MessageTraits<M>::decode(message_buffer, message_);
  }

  std::error_code store(std::span<std::byte> buffer, std::size_t &size) const {
    if (buffer.size() < sizeof(Frame) + message_size_) {
      return std::make_error_code(std::errc::invalid_argument);
    }

    Frame *frame = reinterpret_cast<Frame *>(buffer.data());

    frame->sync = 0x00;
    frame->crc32 = crc32_;
    frame->timestamp = timestamp_;
    frame->sequence = sequence_;
    frame->message_id = message_id_;
    frame->message_size = message_size_;

    std::copy(message_buffer_.begin(), message_buffer_.end(), frame->message);
    size = sizeof(Frame) + message_size_;

    return std::error_code();
  }

  std::uint32_t crc32() const { return crc32_; };
  std::uint64_t timestamp() const { return timestamp_; };
  std::uint16_t sequence() const { return sequence_; }
  MessageId message_id() const { return message_id_; }
  std::size_t message_size() const { return message_size_; }
  M &message() { return message_; }
  const M &message() const { return message_; }

private:
  std::array<std::byte, MessageTraits<M>::SIZE> message_buffer_{};
  std::uint32_t crc32_{0};
  std::uint64_t timestamp_{0};
  std::uint16_t sequence_{0};
  MessageId message_id_{0};
  std::size_t message_size_{0};
  M message_;
};

} // namespace packert

#endif // PACKERT__PACKET_HPP_
