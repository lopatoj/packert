// Copyright 2026 Justin Lopato.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#ifndef PACKERT__PORT_HPP_
#define PACKERT__PORT_HPP_

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <nanopb/pb.h>
#include <span>
#include <system_error>

#include "packert/message.hpp"
#include "packert/packet.hpp"
#include "packert/ring.hpp"
#include "packert/transport.hpp"
#include "packert/visibility_control.h"

namespace packert {

template <std::size_t N> class PACKERT_PUBLIC Port {
public:
  using Decoder = void (*)(const std::span<std::byte> &buffer, void *callback);

  Port(Transport &transport) : transport_(transport) {}

  template <std::derived_from<Message> M, typename Callback>
  std::error_code bind(Callback callback) {
    MessageId message_id = MessageTraits<M>::ID;

    // Passed type without configured nanopb struct
    if (message_id == 0) {
      return std::make_error_code(std::errc::invalid_argument);
    }

    Decoder decoder = [](const std::span<std::byte> &buffer, void *cb) {
      Packet<M> packet;

      if (packet.load(buffer) != std::error_code()) {
        return;
      }
      auto *typed_cb = reinterpret_cast<Callback *>(cb);
      (*typed_cb)(packet);
    };

    for (std::size_t i = 0; i < N; ++i) {
      if (callbacks_[i].message_id == message_id) {
        return std::make_error_code(std::errc::address_in_use);
      }
      if (callbacks_[i].message_id == 0) {
        callbacks_[i].message_id = message_id;
        callbacks_[i].decoder = decoder;
        callbacks_[i].callback = new Callback(std::move(callback));
        return std::error_code();
      }
    }

    return std::make_error_code(std::errc::no_buffer_space);
  }

  template <std::derived_from<Message> M> std::error_code send(const M &m) {
    std::array<std::byte, sizeof(Frame) + MessageTraits<M>::SIZE> out_buffer;
    std::size_t out_buffer_size = 0;

    Packet<M> packet(m);

    if (auto ec = packet.store(out_buffer, out_buffer_size)) {
      return ec;
    }

    transport_.send({out_buffer.data(), out_buffer_size});

    return std::error_code();
  }

  std::error_code poll() {
    std::byte buffer[1024];
    std::size_t received_size = 0;

    auto ec = transport_.receive(buffer, received_size);
    if (ec) {
      return ec;
    }

    std::span<std::byte> packet_buffer = {buffer, received_size};

    const auto &entry = callbacks_[0];
    entry.decoder(packet_buffer, entry.callback);

    return std::error_code();
  }

private:
  struct CallbackEntry {
    MessageId message_id = 0;
    Decoder decoder;
    void *callback;
  };

  std::uint16_t next_send_sequence_ = 0;
  std::uint16_t last_receive_sequence_ = 0;
  Transport &transport_;
  std::array<CallbackEntry, N> callbacks_;
};

} // namespace packert

#endif // PACKERT__PORT__PORT_HPP_
