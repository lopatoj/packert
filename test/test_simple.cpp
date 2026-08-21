#include "packert/message.hpp"
#include "packert/port.hpp"
#include "packert/transport.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <gtest/gtest.h>

using namespace packert;

class SimpleMessage : public packert::Message {
public:
  SimpleMessage() : value_(0) {}
  SimpleMessage(uint32_t value) : value_(value) {}
  std::uint32_t value() const { return value_; }
  void set_value(std::uint32_t value) { value_ = value; }

private:
  std::uint32_t value_;
};

template <> class packert::MessageTraits<SimpleMessage> {
public:
  constexpr static packert::MessageId ID = 1;
  constexpr static std::size_t SIZE = sizeof(std::uint32_t);

  static std::error_code encode(const SimpleMessage &message,
                                std::span<std::byte> buffer,
                                std::size_t &encoded_size) {
    if (buffer.size() < SIZE) {
      return std::make_error_code(std::errc::invalid_argument);
    }
    std::uint32_t *value_ptr = reinterpret_cast<std::uint32_t *>(buffer.data());
    *value_ptr = message.value();
    encoded_size = SIZE;
    return std::error_code();
  }

  static std::error_code decode(std::span<const std::byte> buffer,
                                SimpleMessage &message) {
    if (buffer.size() < SIZE) {
      return std::make_error_code(std::errc::invalid_argument);
    }
    const std::uint32_t *value_ptr =
        reinterpret_cast<const std::uint32_t *>(buffer.data());
    message.set_value(*value_ptr);
    return std::error_code();
  }
};

class SimpleTransport : public packert::Transport {
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

TEST(SimpleTest, Read) {
  SimpleTransport transport;
  auto ec = transport.open();

  if (ec) {
    std::cerr << "Failed to open transport: " << ec.message() << std::endl;
    FAIL();
  }

  // Create a Port with space for one message type
  Port<1> port(transport);

  uint32_t received_value = 0;

  ec = port.bind<SimpleMessage>([&received_value](const auto &packet) {
    std::cout << "Received message with value: " << packet.message().value()
              << std::endl;
    received_value = packet.message().value();
  });
  if (ec) {
    std::cerr << "Failed to bind message: " << ec.message() << std::endl;
    FAIL();
  }

  std::cout << "Sending message..." << std::endl;

  ec = port.send(SimpleMessage{100});
  if (ec) {
    std::cerr << "Failed to send message: " << ec.message() << std::endl;
    FAIL();
  }

  std::cout << "Polling for message..." << std::endl;

  ec = port.poll();
  if (ec) {
    std::cerr << "Failed to poll for message: " << ec.message() << std::endl;
    FAIL();
  }

  ec = transport.close();

  if (ec) {
    std::cerr << "Failed to close transport: " << ec.message() << std::endl;
    FAIL();
  }

  ASSERT_EQ(received_value, 100);
}
