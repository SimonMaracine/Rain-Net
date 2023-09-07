#include <thread>
#include <memory>
#include <string_view>
#include <cstdint>
#include <optional>
#include <string>
#include <iostream>

#define ASIO_NO_DEPRECATED
#include <asio/io_context.hpp>
#include <asio/error_code.hpp>
#include <asio/ip/tcp.hpp>

#include "rain_net/client.hpp"
#include "rain_net/queue.hpp"
#include "rain_net/message.hpp"
#include "rain_net/connection.hpp"

namespace rain_net {
    Client::~Client() {
        disconnect();
    }

    bool Client::connect(std::string_view host, std::uint16_t port) {
        if (asio_context.stopped()) {
            asio_context.restart();
        }

        asio::error_code ec;

        asio::ip::tcp::resolver resolver {asio_context};
        auto endpoints = resolver.resolve(host, std::to_string(port), ec);

        if (ec) {
            std::cout << "Could not resolve host: " << ec.message() << '\n';  // TODO logging

            return false;
        }

        connection = std::make_unique<internal::ServerConnection>(
            &asio_context, &incoming_messages, asio::ip::tcp::socket(asio_context), endpoints
        );

        connection->try_connect();

        context_thread = std::thread([this]() {
            asio_context.run();
        });

        return true;
    }

    void Client::disconnect() {
        if (connection == nullptr) {
            return;
        }

        connection->disconnect();
        asio_context.stop();
        context_thread.join();
        connection.reset();
    }

    bool Client::is_connected() const {
        if (connection == nullptr) {
            return false;
        }

        return connection->is_connected();
    }

    void Client::send_message(const Message& message) {
        if (connection == nullptr) {
            return;
        }

        if (!is_connected()) {
            return;
        }

        connection->send(message);
    }

    std::optional<Message> Client::next_incoming_message() {
        if (incoming_messages.empty()) {
            return std::nullopt;
        }

        return std::make_optional(incoming_messages.pop_front().message);
    }
}
