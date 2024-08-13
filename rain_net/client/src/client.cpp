#include "rain_net/client.hpp"

#include <string>
#include <stdexcept>
#include <utility>

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
        if (m_asio_context.stopped()) {
            m_asio_context.restart();
        }

        asio::ip::tcp::resolver resolver {m_asio_context};

        asio::ip::tcp::resolver::results_type endpoints;

        try {
            endpoints = resolver.resolve(host, std::to_string(port));
        } catch (const std::system_error& e) {
            throw ConnectionError(e.what());
        }

        m_connection = std::make_unique<ServerConnection>(
            m_asio_context,
            asio::ip::tcp::socket(m_asio_context),
            m_incoming_messages,
            endpoints
        );

        m_connection->connect();

        m_context_thread = std::thread([this]() {
            try {
                m_asio_context.run();
            } catch (const std::system_error& e) {
                m_error = std::make_exception_ptr(ConnectionError(e.what()));
            } catch (const ConnectionError& e) {
                m_error = std::current_exception();
            }
        });
    }

    void Client::disconnect() {
        // Don't prime the context, if it has been stopped,
        // because it will do the work after restart, meaning use after free
        if (!m_asio_context.stopped()) {
            if (m_connection != nullptr) {
                m_connection->close();
            }
        }

        if (m_context_thread.joinable()) {
            m_context_thread.join();
        }

        m_connection.reset();

        m_incoming_messages.clear();
    }

    bool Client::connection_established() {
        throw_if_error();

        if (m_connection == nullptr) {
            return false;
        }

        return m_connection->connection_established();
    }

    Message Client::next_message() {
        return m_incoming_messages.pop_front();
    }

    bool Client::available_messages() const {
        return !m_incoming_messages.empty();
    }

    void Client::send_message(const Message& message) {
        throw_if_error();

        if (m_connection == nullptr) {
            return;
        }

        m_connection->send(message);
    }

    void Client::throw_if_error() {
        if (m_error) {
            disconnect();

            const auto error {std::exchange(m_error, nullptr)};
            std::rethrow_exception(error);
        }
    }
}
