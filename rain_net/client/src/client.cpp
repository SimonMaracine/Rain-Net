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
        disconnect();
    }

    void Client::connect(std::string_view host, std::uint16_t port) {
        if (asio_context.stopped()) {
            asio_context.restart();
        }

        asio::ip::tcp::resolver resolver {asio_context};

        asio::ip::tcp::resolver::results_type endpoints;

        try {
            endpoints = resolver.resolve(host, std::to_string(port));
        } catch (const std::system_error& e) {
            throw ConnectionError(e.what());
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
                error = std::make_exception_ptr(ConnectionError(e.what()));
            } catch (const ConnectionError& e) {
                error = std::current_exception();
            }
        });
    }

    void Client::disconnect() {
        // Don't prime the context, if it has been stopped,
        // because it will do the work after restart and meaning use after free
        if (!asio_context.stopped()) {
            if (connection != nullptr) {
                connection->close();
            }
        }

        if (context_thread.joinable()) {
            context_thread.join();
        }

        connection.reset();

        error = nullptr;
    }

    void Client::update() {
        if (error) {
            std::rethrow_exception(error);
        }

        process_messages();
    }

    bool Client::connection_established() const noexcept {
        if (connection == nullptr) {
            return false;
        }

        return connection->connection_established();
    }

    void Client::send_message(const Message& message) {
        assert(connection != nullptr);

        connection->send(message);
    }

    void Client::process_messages() {
        while (!incoming_messages.empty()) {
            on_message_received(incoming_messages.pop_front());
        }
    }
}
