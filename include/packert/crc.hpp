// Copyright 2026 Justin Lopato.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#ifndef PACKERT__CRC_HPP_
#define PACKERT__CRC_HPP_

#include <array>
#include <cstdint>
#include <span>

namespace packert {

class Crc32 {
private:
  static constexpr std::uint32_t POLYNOMIAL = 0xEDB88320;

  static constexpr std::array<std::uint32_t, 256> generate_table() noexcept {
    std::array<std::uint32_t, 256> table{};
    for (std::uint32_t i = 0; i < 256; ++i) {
      std::uint32_t crc = i;
      for (std::uint32_t j = 0; j < 8; ++j) {
        if (crc & 1) {
          crc = (crc >> 1) ^ POLYNOMIAL;
        } else {
          crc >>= 1;
        }
      }
      table[i] = crc;
    }
    return table;
  }

public:
  static uint32_t calculate(std::span<const std::byte> buffer,
                            uint32_t previous_crc = 0) noexcept {
    static constexpr auto table = generate_table();

    uint32_t crc = previous_crc ^ 0xFFFFFFFF;
    const auto *bytes = reinterpret_cast<const std::uint8_t *>(buffer.data());

    for (std::size_t i = 0; i < buffer.size(); ++i) {
      crc = (crc >> 8) ^ table[(crc ^ bytes[i]) & 0xFF];
    }

    return crc ^ 0xFFFFFFFF;
  }
};

} // namespace packert

#endif // PACKERT__CRC_HPP_
