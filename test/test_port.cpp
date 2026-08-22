#include "mocks/message.hpp"
#include "mocks/transport.hpp"
#include "packert/packet.hpp"
#include "packert/port.hpp"

#include <gtest/gtest.h>
#include <cstdint>
#include <cstdio>

TEST(Port, CallbackLimit) {
  NullTransport transport;

  auto ec = transport.open();
  if (ec) {
    std::cerr << "Failed to open transport: " << ec.message() << std::endl;
    FAIL();
  }

  std::cout << "Testing callback limit..." << std::endl;

  packert::Port<2> port(transport);

  std::cout << "Binding first callback..." << std::endl;

  ec = port.bind<Int32Message>([](const packert::Packet<Int32Message> &packet) {
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

  ec = port.bind<StringMessage>(
      [](const packert::Packet<StringMessage> &packet) {
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

TEST(Port, EchoInt32) {
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

TEST(Port, EchoInt32Many) {
  EchoTransport transport;

  auto ec = transport.open();
  if (ec) {
    std::cerr << "Failed to open transport: " << ec.message() << std::endl;
    FAIL();
  }

  packert::Port<1> port(transport);

  std::vector<int32_t> received_values;

  ec = port.bind<Int32Message>([&received_values](const auto &packet) {
    received_values.push_back(packet.message().value());
  });
  if (ec) {
    std::cerr << "Failed to bind message: " << ec.message() << std::endl;
    FAIL();
  }

  for (int32_t i = 0; i < 100; ++i) {
    ec = port.send(Int32Message(i));
    if (ec) {
      std::cerr << "Failed to send message: " << ec.message() << std::endl;
      FAIL();
    }

    ec = port.poll();
    if (ec) {
      std::cerr << "Failed to poll for message: " << ec.message() << std::endl;
      FAIL();
    }
  }

  ec = transport.close();
  if (ec) {
    std::cerr << "Failed to close transport: " << ec.message() << std::endl;
    FAIL();
  }

  ASSERT_EQ(received_values.size(), 100);
  for (int32_t i = 0; i < 100; ++i) {
    ASSERT_EQ(received_values[i], i);
  }
}

TEST(Port, EchoString) {
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

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
