// Copyright 2026 Justin Lopato.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#ifndef PACKERT__MESSAGE_HPP_
#define PACKERT__MESSAGE_HPP_

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <nanopb/pb.h>
#include <span>
#include <system_error>

namespace packert {

using MessageId = std::uint16_t;

constexpr MessageId kInvalidMessageId = 0;

template <typename M> class MessageTraits {
public:
  static constexpr MessageId ID = kInvalidMessageId;
  static constexpr std::size_t SIZE = 0;
  static std::error_code encode(const M & /* message */,
                                std::span<std::byte> /* buffer */,
                                std::size_t & /* encoded_size */) {
    return std::make_error_code(std::errc::not_supported);
  }
  static std::error_code decode(std::span<const std::byte> /* buffer */,
                                M & /* message */) {
    return std::make_error_code(std::errc::not_supported);
  }
};

} // namespace packert

#endif // PACKERT__MESSAGE_HPP_
