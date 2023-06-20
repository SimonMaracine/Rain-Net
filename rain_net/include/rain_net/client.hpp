#pragma once

#include <thread>
#include <memory>
#include <string_view>
#include <cstdint>
#include <string>
#include <iostream>  // TODO logging

#include <asio.hpp>
#include <asio/ts/internet.hpp>

#include "message.hpp"
#include "queue.hpp"

namespace rain_net {
    template<typename E>
    class Client final {
    public:
        Client()
            : temporary_socket(asio_context) {}

        ~Client() {
            disconnect();
        }

        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;
        Client(Client&&) = delete;
        Client& operator=(Client&&) = delete;

        bool connect(std::string_view host, uint16_t port) {
            connection = std::make_unique<Connection<E>>();  // TODO

            asio::error_code ec;

            asio::ip::tcp::resolver resolver {asio_context};
            auto endpoints = resolver.resolve(host, std::to_string(port), ec);

            if (ec) {
                // TODO logging

                std::cout << "Could not resolve host: " << ec.message() << '\n';
                return false;
            }

            connection->connect(endpoints);

            context_thread = std::thread([&asio_context]() {
                asio_context.run();
            });

            return true;
        }

        void disconnect() {
            if (is_connected()) {
                connection->disconnect();
            }

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

        Queue<OwnedMessage<E>> incoming_messages;
    private:
        std::unique_ptr<Connection<E>> connection;

        asio::io_context asio_context;
        std::thread context_thread;

        asio::ip::tcp::socket temporary_socket;  // TODO ?
    };
}
