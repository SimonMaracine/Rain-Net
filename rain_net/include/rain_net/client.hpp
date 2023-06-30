#pragma once

#include <thread>
#include <memory>
#include <string_view>
#include <cstdint>
#include <string>
#include <iostream>  // TODO logging
#include <optional>

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/internet.hpp>

#include "queue.hpp"
#include "message.hpp"
#include "connection.hpp"

namespace rain_net {
    template<typename E>
    class Client {
    public:
        Client() = default;

        virtual ~Client() {
            disconnect();
        }

        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;
        Client(Client&&) = delete;
        Client& operator=(Client&&) = delete;

        bool connect(std::string_view host, uint16_t port) {
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

            connection = std::make_unique<internal::ServerConnection<E>>(
                &asio_context, &incoming_messages, asio::ip::tcp::socket(asio_context), endpoints
            );

            connection->try_connect();

            context_thread = std::thread([this]() {
                asio_context.run();
            });

            return true;
        }

        void disconnect() {
            if (connection == nullptr) {
                return;
            }

            connection->disconnect();
            asio_context.stop();
            context_thread.join();
            connection.reset();
        }

        bool is_connected() const {
            if (connection == nullptr) {
                return false;
            }

            return connection->is_connected();
        }

        void send_message(const Message<E>& message) {
            if (connection == nullptr) {
                return;
            }

            if (!is_connected()) {
                std::cout << "Inconnected\n";
                return;
            }

            connection->send(message);
            std::cout << "Sent message " << message << '\n';
        }

        std::optional<Message<E>> next_incoming_message() {
            if (incoming_messages.empty()) {
                return std::nullopt;
            }

            return std::make_optional(incoming_messages.pop_front().message);
        }

        internal::Queue<internal::OwnedMsg<E>> incoming_messages;
        std::unique_ptr<Connection<E>> connection;
    private:
        asio::io_context asio_context;
        std::thread context_thread;
    };
}
