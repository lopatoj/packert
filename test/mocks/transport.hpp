#pragma once

#include "packert/transport.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <system_error>

class NullTransport : public packert::Transport {
public:
  std::error_code open() override { return std::error_code(); }
  std::error_code close() override { return std::error_code(); }
  std::error_code send(std::span<const std::byte> /* data */) override {
    return std::error_code();
  }
  std::error_code receive(std::span<std::byte> /* data */,
                          std::size_t &size) override {
    size = 0;
    return std::error_code();
  }
};

class EchoTransport : public packert::Transport {
public:
  std::error_code open() override { return std::error_code(); }
  std::error_code close() override { return std::error_code(); }
  std::error_code send(std::span<const std::byte> data) override {
    if (data.size() > storage_.size()) {
      return std::make_error_code(std::errc::invalid_argument);
    }
    std::copy(data.begin(), data.end(), storage_.begin());
    sent_data_ = std::span<std::byte>(storage_.data(), data.size());
    return std::error_code();
  }
  std::error_code receive(std::span<std::byte> data,
                          std::size_t &size) override {
    if (sent_data_.size() > data.size()) {
      return std::make_error_code(std::errc::invalid_argument);
    }

    std::copy(sent_data_.begin(), sent_data_.end(), data.begin());
    size = sent_data_.size();
    return std::error_code();
  }

private:
  std::array<std::byte, 1024> storage_{};
  std::span<std::byte> sent_data_;
};
