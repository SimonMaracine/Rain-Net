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

namespace rain_net {
    namespace internal {
        void PoolClients::create_pool(std::uint32_t pool_size) {
            pool = std::make_unique<bool[]>(pool_size);
            size = pool_size;
            id_pointer = 0;
        }

        std::optional<std::uint32_t> PoolClients::allocate_id() {
            std::lock_guard<std::mutex> lock {mutex};

            const auto result {search_and_allocate_id(id_pointer, size)};

            if (result != std::nullopt) {
                return result;
            }

            // No ID found; start searching from the beginning

            return search_and_allocate_id(0, id_pointer);

            // Return ID or null, if really nothing found
        }

        void PoolClients::deallocate_id(std::uint32_t id) {
            std::lock_guard<std::mutex> lock {mutex};

            assert(pool[id]);

            pool[id] = false;
        }

        std::optional<std::uint32_t> PoolClients::search_and_allocate_id(std::uint32_t begin, std::uint32_t end) {
            for (std::uint32_t id {begin}; id < end; id++) {
                if (!pool[id]) {
                    pool[id] = true;
                    id_pointer = (id + 1) % size;

                    return std::make_optional(id);
                }
            }

            return std::nullopt;
        }
    }

    Server::~Server() {
        stop();
    }

    void Server::start(std::uint16_t port, std::uint32_t max_clients) {
        if (asio_context.stopped()) {
            asio_context.restart();
        }

        clients.create_pool(max_clients);

        const auto endpoint {asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)};

        try {
            acceptor.open(endpoint.protocol());
            acceptor.set_option(asio::socket_base::reuse_address(true));
            acceptor.bind(endpoint);
            acceptor.listen();
        } catch (const std::system_error& e) {
            log_fn("Unexpected error: " + std::string(e.what()));
            set_error(e.what());
            return;
        }

        running = true;

        task_accept_connection();

        context_thread = std::thread([this]() {
            try {
                asio_context.run();
            } catch (const std::system_error& e) {
                log_fn("Unexpected error: " + std::string(e.what()));
                set_error(e.what());
            } catch (const internal::ConnectionError& e) {
                log_fn("Unexpected error: " + std::string(e.what()));
                set_error(e.what());
            }
        });

        log_fn("Server started (port " + std::to_string(port) + ", max " + std::to_string(max_clients) + " clients)");
    }

    void Server::stop() {
        for (const auto& connection : connections) {
            if (connection != nullptr) {
                connection->close();
            }
        }

        running = false;

        if (acceptor.is_open()) {
            acceptor.close();
        }

        if (context_thread.joinable()) {
            context_thread.join();
        }

        for (auto& connection : connections) {
            connection.reset();
        }

        connections.clear();

        clear_error();
    }

    bool Server::available() const {
        return !incoming_messages.empty();
    }

    void Server::accept_connections() {
        // This function may only pop connections and the accepting thread may only push connections

        while (!new_connections.empty()) {
            const auto connection {new_connections.pop_front()};

            if (on_client_connected(connection)) {
                connections.push_front(connection);
                connection->start_communication();
            } else {
                // The server side code must not keep any reference to the connection at this point
                // Must close the socket immediately

                connection->tcp_socket.close();
                clients.deallocate_id(connection->get_id());
            }
        }
    }

    void Server::check_connections() {
        const auto& list {connections};

        for (auto before_iter {list.before_begin()}, iter {list.begin()}; iter != list.end(); before_iter++, iter++) {
            const auto& connection = *iter;

            assert(connection != nullptr);

            if (!connection->is_open()) {
                on_client_disconnected(connection);
                clients.deallocate_id(connection->get_id());
                iter = connections.erase_after(before_iter);

                // If we erased the last element, this check is essential
                if (iter == list.end()) {
                    break;
                }
            }
        }
    }

    std::optional<std::pair<std::shared_ptr<ClientConnection>, Message>> Server::next_incoming_message() {
        if (incoming_messages.empty()) {
            return std::nullopt;
        }

        const auto owned_msg {incoming_messages.pop_front()};

        assert(owned_msg.remote != nullptr);

        return std::make_optional(std::make_pair(owned_msg.remote, owned_msg.message));
    }

    void Server::send_message(std::shared_ptr<ClientConnection> connection, const Message& message) {
        assert(connection != nullptr);

        if (!connection->is_open()) {
            on_client_disconnected(connection);
            clients.deallocate_id(connection->get_id());
            connections.remove(connection);
            return;
        }

        connection->send(message);
    }

    void Server::send_message_broadcast(const Message& message) {
        const auto& list {connections};

        for (auto before_iter {list.before_begin()}, iter {list.begin()}; iter != list.end(); before_iter++, iter++) {
            const auto& connection {*iter};

            assert(connection != nullptr);

            if (!connection->is_open()) {
                on_client_disconnected(connection);
                clients.deallocate_id(connection->get_id());
                iter = connections.erase_after(before_iter);

                // If we erased the last element, this check is essential
                if (iter == list.end()) {
                    break;
                }

                continue;
            }

            connection->send(message);
        }
    }

    void Server::send_message_broadcast(const Message& message, std::shared_ptr<ClientConnection> exception) {
        const auto& list {connections};

        for (auto before_iter {list.before_begin()}, iter {list.begin()}; iter != list.end(); before_iter++, iter++) {
            const auto& connection {*iter};

            assert(connection != nullptr);

            if (connection == exception) {
                continue;
            }

            if (!connection->is_open()) {
                on_client_disconnected(connection);
                clients.deallocate_id(connection->get_id());
                iter = connections.erase_after(before_iter);

                // If we erased the last element, this check is essential
                if (iter == list.end()) {
                    break;
                }

                continue;
            }

            connection->send(message);
        }
    }

    void Server::task_accept_connection() {
        // Do note that in this thread IDs are allocated, but in the main thread they are freed

        acceptor.async_accept(
            [this](asio::error_code ec, asio::ip::tcp::socket socket) {
                if (ec) {
                    log_fn("Could not accept new connection: " + ec.message());
                } else {
                    log_fn(
                        "Accepted new connection: " +
                        socket.remote_endpoint().address().to_string() +
                        ", " +
                        std::to_string(socket.remote_endpoint().port())
                    );

                    const auto new_id {clients.allocate_id()};

                    if (!new_id) {
                        socket.close();

                        log_fn("Actively rejected connection; ran out of IDs");
                    } else {
                        new_connections.push_back(
                            std::make_shared<ClientConnection>(
                                asio_context,
                                std::move(socket),
                                incoming_messages,
                                *new_id,
                                log_fn
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
}
