// Copyright 2026 Justin Lopato.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#ifndef PACKERT__SERIAL__COBS_HPP_
#define PACKERT__SERIAL__COBS_HPP_

#include <cstddef>
#include <cstdint>

namespace packert {

constexpr std::size_t max_encoded_size(std::size_t src_size) noexcept {
  return src_size + (src_size / 254) + 1;
}

inline std::size_t encode(const std::uint8_t *src, std::size_t src_size,
                          std::uint8_t *dst,
                          std::size_t dst_capacity) noexcept {
  if (dst_capacity < max_encoded_size(src_size)) {
    return 0;
  }

  std::size_t write = 1;
  std::size_t code_idx = 0;
  std::uint8_t code = 0x01;

  for (std::size_t read = 0; read < src_size; ++read) {
    const bool is_zero = (src[read] == 0x00);
    if (!is_zero) {
      dst[write++] = src[read];
      ++code;
    }
    if (is_zero || code == 0xFF) {
      dst[code_idx] = code;
      code = 0x01;
      code_idx = write;
      if (is_zero || read + 1 < src_size) {
        ++write;
      }
    }
  }
  dst[code_idx] = code;
  return write;
}

inline std::size_t decode(const std::uint8_t *src, std::size_t src_size,
                          std::uint8_t *dst,
                          std::size_t dst_capacity) noexcept {
  if (src_size == 0) {
    return 0;
  }

  std::size_t read = 0;
  std::size_t write = 0;

  while (read < src_size) {
    const std::uint8_t code = src[read++];
    if (code == 0x00) {
      return 0;
    }
    for (std::uint8_t i = 1; i < code; ++i) {
      if (read >= src_size) {
        return 0;
      }
      if (write >= dst_capacity) {
        return 0;
      }
      dst[write++] = src[read++];
    }
    if (code != 0xFF && read < src_size) {
      if (write >= dst_capacity) {
        return 0;
      }
      dst[write++] = 0x00;
    }
  }

  return write;
}

} // namespace packert

#endif // PACKERT__SERIAL__COBS_HPP_
