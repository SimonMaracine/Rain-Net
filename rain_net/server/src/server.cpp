#include "rain_net/server.hpp"

#include <stdexcept>
#include <cassert>

#ifdef __GNUG__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wconversion"
#endif

#include <asio/error_code.hpp>

#ifdef __GNUG__
    #pragma GCC diagnostic pop
#endif

#include "rain_net/internal/error.hpp"

using namespace std::string_literals;

namespace rain_net {
    Server::~Server() {
        stop();
    }

    void Server::start(std::uint16_t port, std::uint32_t max_clients) {
        if (asio_context.stopped()) {
            asio_context.restart();
        }

        pool.create(max_clients);

        const auto endpoint {asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)};

        try {
            acceptor.open(endpoint.protocol());
            acceptor.bind(endpoint);
            acceptor.listen();
        } catch (const std::system_error& e) {
            on_log("Unexpected error: "s + e.what());
            throw ConnectionError(e.what());
        }

        running = true;

        task_accept_connection();

        context_thread = std::thread([this]() {
            try {
                asio_context.run();
            } catch (const std::system_error& e) {
                on_log("Unexpected error: "s + e.what());
                error = std::make_exception_ptr(ConnectionError(e.what()));
            } catch (const ConnectionError& e) {
                on_log("Unexpected error: "s + e.what());
                error = std::current_exception();
            }
        });

        on_log("Server started (port " + std::to_string(port) + ", max " + std::to_string(max_clients) + " clients)");
    }

    void Server::stop() {
        running = false;

        // Don't prime the context, if it has been stopped,
        // because it will do the work after restart, meaning use after free
        if (!asio_context.stopped()) {
            for (const auto& connection : connections) {
                if (connection != nullptr) {
                    connection->close();
                }
            }
        }

        if (acceptor.is_open()) {
            try {
                acceptor.close();
            } catch (const std::system_error&) {}
        }

        if (context_thread.joinable()) {
            context_thread.join();
        }

        for (auto& connection : connections) {
            connection.reset();
        }

        connections.clear();

        new_connections.clear();

        incoming_messages.clear();

        error = nullptr;
    }

    void Server::accept_connections() {
        if (error) {
            std::rethrow_exception(error);
        }

        // This function may only pop connections and the accepting thread may only push connections

        while (!new_connections.empty()) {
            const auto connection {new_connections.pop_front()};

            if (on_client_connected(*this, connection)) {
                connections.push_front(connection);
                connection->start_communication();
            } else {
                // The server side code must not keep any reference to the connection at this point
                // Must close the socket immediately

                connection->tcp_socket.close();
                pool.deallocate_id(connection->get_id());
            }
        }
    }

    std::pair<Message, std::shared_ptr<ClientConnection>> Server::next_message() {
        if (error) {
            std::rethrow_exception(error);
        }

        return incoming_messages.pop_front();
    }

    bool Server::available_messages() const {
        return !incoming_messages.empty();
    }

    void Server::check_connections() {
        if (error) {
            std::rethrow_exception(error);
        }

        for (auto before_iter {connections.before_begin()}, iter {connections.begin()}; iter != connections.end(); before_iter++, iter++) {
            const auto& connection {*iter};

            assert(connection != nullptr);

            if (!connection->is_open()) {
                if (maybe_client_disconnected(connection, iter, before_iter)) {
                    break;
                }
            }
        }
    }

    void Server::send_message(std::shared_ptr<ClientConnection> connection, const Message& message) {
        assert(connection != nullptr);

        if (!connection->is_open()) {
            maybe_client_disconnected(connection);
            return;
        }

        connection->send(message);
    }

    void Server::send_message_broadcast(const Message& message) {
        for (auto before_iter {connections.before_begin()}, iter {connections.begin()}; iter != connections.end(); before_iter++, iter++) {
            const auto& connection {*iter};

            assert(connection != nullptr);

            if (!connection->is_open()) {
                if (maybe_client_disconnected(connection, iter, before_iter)) {
                    break;
                }

                continue;
            }

            connection->send(message);
        }
    }

    void Server::send_message_broadcast(const Message& message, std::shared_ptr<ClientConnection> exception) {
        for (auto before_iter {connections.before_begin()}, iter {connections.begin()}; iter != connections.end(); before_iter++, iter++) {
            const auto& connection {*iter};

            assert(connection != nullptr);

            if (connection == exception) {
                continue;
            }

            if (!connection->is_open()) {
                if (maybe_client_disconnected(connection, iter, before_iter)) {
                    break;
                }

                continue;
            }

            connection->send(message);
        }
    }

    void Server::task_accept_connection() {
        // In this thread IDs are allocated, but in the main thread they are freed

        acceptor.async_accept(
            [this](asio::error_code ec, asio::ip::tcp::socket socket) {
                if (ec) {
                    on_log("Could not accept new connection: " + ec.message());
                } else {
                    on_log(
                        "Accepted new connection: " +
                        socket.remote_endpoint().address().to_string() +
                        ", " +
                        std::to_string(socket.remote_endpoint().port())
                    );

                    const auto new_id {pool.allocate_id()};

                    if (!new_id) {
                        socket.close();

                        on_log("Actively rejected connection; ran out of IDs");
                    } else {
                        new_connections.push_back(
                            std::make_shared<ClientConnection>(
                                asio_context,
                                std::move(socket),
                                incoming_messages,
                                *new_id,
                                on_log
                            )
                        );
                    }
                }

                if (!running) {
                    return;
                }

                task_accept_connection();
            }
        );
    }

    void Server::maybe_client_disconnected(std::shared_ptr<ClientConnection> connection) {
        if (connection->used) {
            return;
        }

        on_client_disconnected(*this, connection);
        pool.deallocate_id(connection->get_id());
        connections.remove(connection);

        connection->used = true;
    }

    bool Server::maybe_client_disconnected(std::shared_ptr<ClientConnection> connection, ConnectionsIter& iter, ConnectionsIter before_iter) {
        if (connection->used) {
            return false;
        }

        on_client_disconnected(*this, connection);
        pool.deallocate_id(connection->get_id());

        connection->used = true;

        return (iter = connections.erase_after(before_iter)) == connections.end();
    }
}
