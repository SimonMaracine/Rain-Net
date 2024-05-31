#include "rain_net/client.hpp"

#include <iostream>
#include <string>
#include <stdexcept>

#ifdef __GNUG__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wconversion"
#endif

#include <asio/error_code.hpp>
#include <asio/ip/tcp.hpp>

#ifdef __GNUG__
    #pragma GCC diagnostic pop
#endif

namespace rain_net {
    bool Client::connect(std::string_view host, std::uint16_t port) {
        if (asio_context.stopped()) {
            asio_context.restart();
        }

        asio::error_code ec;

        asio::ip::tcp::resolver resolver {asio_context};
        const auto endpoints {resolver.resolve(host, std::to_string(port), ec)};

        if (ec) {
            std::cout << "Could not resolve host: " << ec.message() << '\n';  // TODO logging

            return false;
        }

        connection = std::make_unique<ServerConnection>(
            &asio_context,
            asio::ip::tcp::socket(asio_context),
            &incoming_messages,
            endpoints
        );

        connection->connect();

        context_thread = std::thread([this]() {
            try {
                asio_context.run();
            } catch (const std::system_error& e) {
                std::cout << "Critical error: " << e.what() << "\n";
            }
        });

        return true;
    }

    void Client::disconnect() {
        connection->close();
        context_thread.join();
        connection.reset();
    }

    bool Client::is_connected() const {
        if (connection == nullptr) {
            return false;
        }

        return connection->is_connected();
    }

    bool Client::is_socket_open() const {
        if (connection == nullptr) {
            return false;
        }

        return connection->is_open();
    }

    bool Client::send_message(const Message& message) {
        if (!is_socket_open()) {
            return false;
        }

        connection->send(message);

        return true;
    }

    std::optional<Message> Client::next_incoming_message() {
        if (incoming_messages.empty()) {
            return std::nullopt;
        }

        return std::make_optional(incoming_messages.pop_front());
    }
}
