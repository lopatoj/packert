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
#include <span>
#include <system_error>

namespace packert {

/**
 * @brief Hardware abstraction layer for an arbitrary communication interface.
 * @todo Migrate to API for asynchronous communication.
 */
class Transport {
public:
  virtual ~Transport() = default;
  virtual std::error_code open() = 0;
  virtual std::error_code close() = 0;
  virtual std::error_code send(std::span<const std::byte> data) = 0;
  virtual std::error_code receive(std::span<std::byte> data,
                                  std::size_t &size) = 0;
};

} // namespace packert

#endif // PACKERT__TRANSPORT_HPP_
