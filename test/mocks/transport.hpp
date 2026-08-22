#pragma once

#include "packert/transport.hpp"

#include <cstddef>
#include <expected>
#include <queue>
#include <span>
#include <system_error>
#include <vector>

class NullTransport : public packert::Transport {
 public:
  std::error_code open() { return {}; }
  std::error_code close() { return {}; }
  std::error_code send(std::span<const std::byte> /* buffer */) override {
    return {};
  }
  std::expected<managed_buffer, std::error_code> receive() override {
    return managed_buffer{*this, std::span<const std::byte>{}, 0};
  }
  void relinquish(managed_buffer & /* buffer */) override {}
};

class EchoTransport : public packert::Transport {
 public:
  std::error_code open() { return {}; }
  std::error_code close() { return {}; }
  std::error_code send(std::span<const std::byte> buffer) override {
    if (buffer.size() == 0) {
      return {};
    }

    queue_.push(std::vector<std::byte>(buffer.begin(), buffer.end()));
    return {};
  }

  std::expected<managed_buffer, std::error_code> receive() override {
    if (queue_.empty()) {
      return std::unexpected(
          std::make_error_code(std::errc::no_message_available));
    }

    buffer_pool_[next_index_] = std::move(queue_.front());
    queue_.pop();
    managed_buffer buffer{*this,
                          std::span<const std::byte>(buffer_pool_[next_index_]),
                          next_index_};
    buffer_pool_used_[next_index_] = true;

    for (std::uint16_t i = 0; i < 32; ++i) {
      if (!buffer_pool_used_[i]) {
        next_index_ = i;
        break;
      }
    }

    return buffer;
  }

  void relinquish(managed_buffer &buffer) override {
    if (buffer.id() < 32) {
      buffer_pool_[buffer.id()].clear();
      buffer_pool_used_[buffer.id()] = false;
    }
  }

 private:
  std::queue<std::vector<std::byte>> queue_;
  std::array<std::vector<std::byte>, 32> buffer_pool_;
  std::array<bool, 32> buffer_pool_used_;
  std::uint16_t next_index_ = 0;
};
