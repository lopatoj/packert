#include "mocks/message.hpp"
#include "mocks/transport.hpp"
#include "packert/message.hpp"
#include "packert/port.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <gtest/gtest.h>

TEST(SimpleTest, CallbackLimit) {
  NullTransport transport;

  auto ec = transport.open();
  if (ec) {
    std::cerr << "Failed to open transport: " << ec.message() << std::endl;
    FAIL();
  }

  std::cout << "Testing callback limit..." << std::endl;

  packert::Port<2> port(transport);

  std::cout << "Binding first callback..." << std::endl;

  ec = port.bind<Int32Message>([](const auto &packet) {
    std::cout << "Received message with value: " << packet.message().value()
              << ", id: " << packet.message_id() << std::endl;
  });
  if (ec) {
    std::cerr << "Failed to bind message: " << ec.message()
              << ", id: " << packert::MessageTraits<Int32Message>::ID
              << std::endl;
    FAIL();
  }

  std::cout << "Binding second callback..." << std::endl;

  ec = port.bind<StringMessage>([](const auto &packet) {
    std::cout << "Received message with value: " << packet.message().value()
              << ", id: " << packet.message_id() << std::endl;
  });
  if (ec) {
    std::cerr << "Failed to bind message: " << ec.message()
              << ", id: " << packert::MessageTraits<StringMessage>::ID
              << std::endl;
    FAIL();
  }

  std::cout << "Binding third callback (should fail)..." << std::endl;

  ec = port.bind<Int32Message>([](const auto &packet) {
    std::cout << "Received message with value: " << packet.message().value()
              << ", id: " << packet.message_id() << std::endl;
  });
  if (!ec) {
    std::cerr
        << "Expected error when binding more than 2 callbacks, but got success."
        << std::endl;
    FAIL();
  } else {
    std::cout << "Received expected error when binding more than 2 callbacks: "
              << ec.message() << std::endl;
  }

  ec = transport.close();
  if (ec) {
    std::cerr << "Failed to close transport: " << ec.message() << std::endl;
    FAIL();
  }
}

TEST(SimpleTest, EchoInt32) {
  EchoTransport transport;

  auto ec = transport.open();
  if (ec) {
    std::cerr << "Failed to open transport: " << ec.message() << std::endl;
    FAIL();
  }

  packert::Port<1> port(transport);

  uint32_t received_value = 0;

  ec = port.bind<Int32Message>([&received_value](const auto &packet) {
    std::cout << "Received message with value: " << packet.message().value()
              << ", id: " << packet.message_id() << std::endl;
    received_value = packet.message().value();
  });
  if (ec) {
    std::cerr << "Failed to bind message: " << ec.message() << std::endl;
    FAIL();
  }

  std::cout << "Sending message..." << std::endl;

  ec = port.send<Int32Message>({100});
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

TEST(SimpleTest, EchoString) {
  EchoTransport transport;

  auto ec = transport.open();
  if (ec) {
    std::cerr << "Failed to open transport: " << ec.message() << std::endl;
    FAIL();
  }

  packert::Port<1> port(transport);

  std::string received_value;

  ec = port.bind<StringMessage>([&received_value](const auto &packet) {
    std::cout << "Received message with value: " << packet.message().value()
              << ", id: " << packet.message_id() << std::endl;
    received_value = packet.message().value();
  });
  if (ec) {
    std::cerr << "Failed to bind message: " << ec.message() << std::endl;
    FAIL();
  }

  std::cout << "Sending message..." << std::endl;

  ec = port.send<StringMessage>({"Hello, World!"});
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

  ASSERT_EQ(received_value, "Hello, World!");
}
