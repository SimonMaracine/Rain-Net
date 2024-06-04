#include "rain_net/client.hpp"

#include <string>
#include <stdexcept>
#include <cassert>

#ifdef __GNUG__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wconversion"
#endif

#include <asio/error_code.hpp>
#include <asio/ip/tcp.hpp>

#ifdef __GNUG__
    #pragma GCC diagnostic pop
#endif

#include "rain_net/internal/error.hpp"

namespace rain_net {
    Client::~Client() {
        if (connection != nullptr) {
            connection->close();
        }

        if (context_thread.joinable()) {
            context_thread.join();
        }
    }

    void Client::connect(std::string_view host, std::uint16_t port) {
        if (asio_context.stopped()) {
            asio_context.restart();
        }

        asio::error_code ec;

        asio::ip::tcp::resolver resolver {asio_context};
        const auto endpoints {resolver.resolve(host, std::to_string(port), ec)};

        if (ec) {
            set_error("Could not resolve host: " + ec.message());
            return;
        }

        connection = std::make_unique<ServerConnection>(
            asio_context,
            asio::ip::tcp::socket(asio_context),
            incoming_messages,
            endpoints
        );

        connection->connect();

        context_thread = std::thread([this]() {
            try {
                asio_context.run();
            } catch (const std::system_error& e) {
                set_error(e.what());
            } catch (const internal::ConnectionError& e) {
                set_error(e.what());
            }
        });
    }

    void Client::disconnect() {
        assert(connection != nullptr);

        connection->close();
        context_thread.join();
        connection.reset();
    }

    bool Client::connection_established() const {
        if (connection == nullptr) {
            return false;
        }

        return connection->connection_established();
    }

    bool Client::is_socket_open() const {
        if (connection == nullptr) {
            return false;
        }

        return connection->is_open();
    }

    void Client::send_message(const Message& message) {
        assert(connection != nullptr);

        connection->send(message);
    }

    std::optional<Message> Client::next_incoming_message() {
        if (incoming_messages.empty()) {
            return std::nullopt;
        }

        return std::make_optional(incoming_messages.pop_front());
    }
}
