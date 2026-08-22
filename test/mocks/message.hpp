#pragma once

#include "packert/message.hpp"

#include <cstdint>
#include <cstring>
#include <span>
#include <system_error>

class Int32Message {
 public:
  Int32Message() : value_(0) {}
  Int32Message(int32_t value) : value_(value) {}
  std::int32_t value() const { return value_; }

 private:
  std::int32_t value_;
};

template <>
class packert::MessageTraits<Int32Message> {
 public:
  constexpr static packert::MessageId ID = 1;
  constexpr static std::size_t SIZE = sizeof(std::int32_t);

  static std::error_code encode(const Int32Message &message,
                                std::span<std::byte> buffer,
                                std::size_t &encoded_size) {
    if (buffer.size() < SIZE) {
      return std::make_error_code(std::errc::invalid_argument);
    }
    std::int32_t *value_ptr = reinterpret_cast<std::int32_t *>(buffer.data());
    *value_ptr = message.value();
    encoded_size = SIZE;
    return std::error_code();
  }

  static std::error_code decode(std::span<const std::byte> buffer,
                                Int32Message &message) {
    if (buffer.size() < SIZE) {
      return std::make_error_code(std::errc::invalid_argument);
    }
    const std::int32_t *value_ptr =
        reinterpret_cast<const std::int32_t *>(buffer.data());
    message = Int32Message{*value_ptr};
    return std::error_code();
  }
};

class StringMessage {
 public:
  StringMessage() : value_("") {}
  StringMessage(const char *value) : value_(value) {}
  const char *value() const { return value_; }

 private:
  const char *value_;
};

template <>
class packert::MessageTraits<StringMessage> {
 public:
  constexpr static packert::MessageId ID = 2;
  constexpr static std::size_t SIZE = 256;

  static std::error_code encode(const StringMessage &message,
                                std::span<std::byte> buffer,
                                std::size_t &encoded_size) {
    if (buffer.size() < SIZE) {
      return std::make_error_code(std::errc::invalid_argument);
    }
    memcpy(buffer.data(), message.value(), strlen(message.value()));
    encoded_size = SIZE;
    return std::error_code();
  }

  static std::error_code decode(std::span<const std::byte> buffer,
                                StringMessage &message) {
    if (buffer.size() < SIZE) {
      return std::make_error_code(std::errc::invalid_argument);
    }
    message = {reinterpret_cast<const char *>(buffer.data())};
    return std::error_code();
  }
};
