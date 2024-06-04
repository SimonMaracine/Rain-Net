#include "rain_net/server.hpp"

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
        PoolClients::PoolClients(std::uint32_t size) {
            create_pool(size);
        }

        std::optional<std::uint32_t> PoolClients::allocate_id() {
            const auto result {search_and_allocate_id(id_pointer, size)};

            if (result != std::nullopt) {
                return result;
            }

            // No ID found; start searching from the beginning

            return search_and_allocate_id(0, id_pointer);

            // Returns ID or null, if really nothing found
        }

        void PoolClients::deallocate_id(std::uint32_t id) {
            assert(pool[id]);

            pool[id] = false;
        }

        void PoolClients::create_pool(std::uint32_t size) {
            pool = std::make_unique<bool[]>(size);
            this->size = size;
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
        for (const auto& connection : active_connections) {
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
    }

    void Server::start(std::uint32_t max_clients) {
        if (asio_context.stopped()) {
            asio_context.restart();
        }

        clients = internal::PoolClients(max_clients);

        running = true;

        task_accept_connection();

        context_thread = std::thread([this]() {
            try {
                asio_context.run();
            } catch (const std::system_error& e) {
                log_fn("Unexpected error: " + std::string(e.what()));

                set_error(e.what());
            } catch (const ConnectionError& e) {
                log_fn("Unexpected error: " + std::string(e.what()));

                set_error(e.what());
            }
        });

        log_fn("Server started (max " + std::to_string(max_clients) + " clients)");
    }

    void Server::stop() {
        for (const auto& connection : active_connections) {
            assert(connection != nullptr);

            connection->close();
        }

        running = false;

        acceptor.close();
        context_thread.join();

        for (auto& connection : active_connections) {
            connection.reset();
        }

        log_fn("Server stopped");
    }

    std::optional<std::pair<std::shared_ptr<ClientConnection>, Message>> Server::next_incoming_message() {
        if (incoming_messages.empty()) {
            return std::nullopt;
        }

        const auto owned_msg {incoming_messages.pop_front()};

        assert(owned_msg.remote != nullptr);

        return std::make_optional(std::make_pair(owned_msg.remote, owned_msg.message));
    }

    bool Server::available() const {
        return !incoming_messages.empty();
    }

    void Server::send_message(std::shared_ptr<ClientConnection> connection, const Message& message) {
        assert(connection != nullptr);

        if (!connection->is_open()) {
            on_client_disconnected(connection);
            clients.deallocate_id(connection->get_id());
            active_connections.remove(connection);
            return;
        }

        connection->send(message);
    }

    void Server::send_message_broadcast(const Message& message) {
        const auto& list {active_connections};

        for (auto before_iter {list.before_begin()}, iter {list.begin()}; iter != list.end(); before_iter++, iter++) {
            const auto& connection {*iter};

            assert(connection != nullptr);

            if (!connection->is_open()) {
                on_client_disconnected(connection);
                clients.deallocate_id(connection->get_id());
                iter = active_connections.erase_after(before_iter);

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
        const auto& list {active_connections};

        for (auto before_iter {list.before_begin()}, iter {list.begin()}; iter != list.end(); before_iter++, iter++) {
            const auto& connection {*iter};

            assert(connection != nullptr);

            if (connection == exception) {
                continue;
            }

            if (!connection->is_open()) {
                on_client_disconnected(connection);
                clients.deallocate_id(connection->get_id());
                iter = active_connections.erase_after(before_iter);

                // If we erased the last element, this check is essential
                if (iter == list.end()) {
                    break;
                }

                continue;
            }

            connection->send(message);
        }
    }

    void Server::check_connections() {
        const auto& list {active_connections};

        for (auto before_iter {list.before_begin()}, iter {list.begin()}; iter != list.end(); before_iter++, iter++) {
            const auto& connection = *iter;

            assert(connection != nullptr);

            if (!connection->is_open()) {
                on_client_disconnected(connection);
                clients.deallocate_id(connection->get_id());
                iter = active_connections.erase_after(before_iter);

                // If we erased the last element, this check is essential
                if (iter == list.end()) {
                    break;
                }
            }
        }
    }

    void Server::task_accept_connection() {
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
                        log_fn("Actively rejected connection; pool is full");

                        socket.close();
                    } else {
                        create_new_connection(std::move(socket), *new_id);
                    }
                }

                if (!running) {
                    return;
                }

                task_accept_connection();
            }
        );
    }

    void Server::create_new_connection(asio::ip::tcp::socket&& socket, std::uint32_t id) {
        const auto connection {std::make_shared<ClientConnection>(
            asio_context,
            std::move(socket),
            incoming_messages,
            id,
            log_fn
        )};

        if (on_client_connected(connection)) {
            active_connections.push_front(connection);
            connection->start_communication();

            log_fn("[" + std::to_string(id) + "] Approved connection");
        } else {
            connection->close();
            clients.deallocate_id(id);  // Take back the unused ID

            log_fn("Actively rejected connection from server side code");
        }
    }
}
