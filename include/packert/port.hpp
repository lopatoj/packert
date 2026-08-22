// Copyright 2026 Justin Lopato.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#ifndef PACKERT__PORT_HPP_
#define PACKERT__PORT_HPP_

#include <nanopb/pb.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <system_error>

#include "packert/message.hpp"
#include "packert/packet.hpp"
#include "packert/transport.hpp"
#include "packert/visibility_control.h"

namespace packert {

template <std::size_t N>
class PACKERT_PUBLIC Port {
 public:
  using Decoder = std::error_code (*)(std::span<const std::byte> buffer,
                                      void *callback);

  Port(Transport &transport) : transport_(transport) {}

  template <typename M, typename Callback>
    requires HasTraits<M> && std::is_invocable_v<Callback, const Packet<M> &>
  std::error_code bind(Callback callback) {
    MessageId message_id = MessageTraits<M>::ID;

    // Passed type without configured nanopb struct
    if (message_id == kInvalidMessageId) {
      return std::make_error_code(std::errc::invalid_argument);
    }

    Decoder decoder = [](std::span<const std::byte> buffer, void *cb) {
      Packet<M> packet;

      if (auto ec = packet.load(buffer)) {
        return ec;
      }

      auto *typed_cb = reinterpret_cast<Callback *>(cb);
      (*typed_cb)(packet);

      return std::error_code();
    };

    for (std::size_t i = 0; i < N; ++i) {
      if (callbacks_[i].message_id == message_id) {
        return std::make_error_code(std::errc::address_in_use);
      }
      if (callbacks_[i].message_id == kInvalidMessageId) {
        callbacks_[i].message_id = message_id;
        callbacks_[i].decoder = decoder;
        callbacks_[i].callback = new Callback(std::move(callback));
        return std::error_code();
      }
    }

    return std::make_error_code(std::errc::no_buffer_space);
  }

  template <typename M>
    requires HasTraits<M>
  std::error_code send(const M &m) {
    Packet<M> packet(m);

    auto ex = packet.store(send_buffer_);
    if (!ex) {
      return ex.error();
    }

    return transport_.send(ex.value());
  }

  std::error_code poll() {
    // Attempt to send an empty frame to flush the receive buffer.
    if (auto ec = transport_.send({})) {
      return ec;
    }

    for (auto i = 0U; i < max_receive_count_; i++) {
      auto ex = transport_.receive();
      if (!ex) {
        auto error = ex.error();

        // No message available error code indicates call succeeded.
        return error == std::errc::no_message_available ? std::error_code()
                                                        : error;
      }

      auto &buffer = ex.value();

      // Malformed packet, return error code.
      if (buffer.size() < sizeof(Frame)) {
        return std::make_error_code(std::errc::bad_message);
      }

      const Frame *frame = reinterpret_cast<const Frame *>(buffer.data());

      // Check for available callback for message type.
      MessageId message_id = frame->message_id;
      for (const auto &entry : callbacks_) {
        if (entry.message_id == message_id) {
          if (auto ec = entry.decoder(buffer.span(), entry.callback)) {
            return ec;
          }
          break;
        }
      }
    }

    return {};
  }

 private:
  struct CallbackEntry {
    MessageId message_id = 0;
    Decoder decoder;
    void *callback;
  };

  std::uint16_t next_send_sequence_ = 0;
  std::uint16_t last_receive_sequence_ = 0;
  std::size_t max_receive_count_ = 10;
  Transport &transport_;
  std::array<CallbackEntry, N> callbacks_;

  std::array<std::byte, 1024> send_buffer_;
};

}  // namespace packert

#endif  // PACKERT__PORT__PORT_HPP_
