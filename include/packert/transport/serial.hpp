// Copyright 2026 Justin Lopato.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#ifndef PACKERT__SERIAL__SERIAL_HPP_
#define PACKERT__SERIAL__SERIAL_HPP_

#include "packert/port.hpp"
#include "packert/ring.hpp"
#include "packert/transport.hpp"

namespace packert {

class Serial : public Transport {
public:
  static constexpr std::size_t kBufferCapacity = 4096;
  static constexpr std::size_t kMaxPacketSize = 1024;

  SerialPort();
  ~SerialPort() override;

  SerialPort(const SerialPort &) = delete;
  SerialPort &operator=(const SerialPort &) = delete;
  SerialPort(SerialPort &&) = delete;
  SerialPort &operator=(SerialPort &&) = delete;

  std::error_code open() override;
  std::error_code close() override;
  bool is_open() const override;
  std::error_code send(const std::uint8_t *data, std::size_t size) override;
  std::error_code flush() override;
  std::error_code poll() override;
  bool has_packet() const override;
  std::error_code receive(std::uint8_t *data, std::size_t capacity,
                          std::size_t &size_out) override;

private:
  int fd_ = -1;
  bool is_open_ = false;
  std::size_t pending_packets_ = 0;

  RingBuffer<kBufferCapacity> send_buffer_;
  RingBuffer<kBufferCapacity> receive_buffer_;
};

} // namespace packert

#endif // PACKERT__SERIAL__SERIAL_HPP_
