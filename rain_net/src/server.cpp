#include "rain_net/server.hpp"

#include <ostream>
#include <utility>
#include <algorithm>
#include <cassert>

#ifdef __GNUG__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wconversion"
#endif

#include <asio/error_code.hpp>

#ifdef __GNUG__
    #pragma GCC diagnostic pop
#endif

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
                if (stream) {
                    *stream << "Critical error: " << e.what() << "\n";
                }
            }
        });

        if (stream) {
            *stream << "Server started with max " << max_clients << " clients\n";
        }
    }

    void Server::stop() {
        for (const auto& connection : active_connections) {
            connection->close();
        }

        running = false;

        acceptor.cancel();
        context_thread.join();

        if (stream) {
            *stream << "Server stopped\n";
        }
    }

    void Server::update(std::uint32_t max_messages, bool wait) {
        // if (wait) {
        //     incoming_messages.wait();  // FIXME
        // }

        std::uint32_t messages_processed {0};

        while (!incoming_messages.empty() && messages_processed < max_messages) {
            const auto owned_msg {incoming_messages.pop_front()};

            assert(owned_msg.remote != nullptr);

            on_message_received(owned_msg.remote, owned_msg.message);

            messages_processed++;
        }
    }

    bool Server::available() const {
        return !incoming_messages.empty();
    }

    bool Server::send_message(std::shared_ptr<ClientConnection> client_connection, const Message& message) {
        assert(client_connection != nullptr);

        if (!client_connection->is_open()) {
            on_client_disconnected(client_connection);
            clients.deallocate_id(client_connection->get_id());
            active_connections.remove(client_connection);

            return false;
        } else {
            client_connection->send(message);

            return true;
        }
    }

    void Server::send_message_broadcast(const Message& message) {
        const auto& list {active_connections};

        for (auto before_iter {list.before_begin()}, iter {list.begin()}; iter != list.end(); before_iter++, iter++) {
            const auto& client_connection {*iter};

            assert(client_connection != nullptr);

            if (!client_connection->is_open()) {
                on_client_disconnected(client_connection);
                clients.deallocate_id(client_connection->get_id());
                iter = active_connections.erase_after(before_iter);

                // If we erased the last element, this check is essential
                if (iter == list.end()) {
                    break;
                }
            } else {
                client_connection->send(message);
            }
        }
    }

    void Server::send_message_broadcast(const Message& message, std::shared_ptr<ClientConnection> exception) {
        const auto& list {active_connections};

        for (auto before_iter {list.before_begin()}, iter {list.begin()}; iter != list.end(); before_iter++, iter++) {
            const auto& client_connection {*iter};

            assert(client_connection != nullptr);

            if (client_connection == exception) {
                continue;
            }

            if (!client_connection->is_open()) {
                on_client_disconnected(client_connection);
                clients.deallocate_id(client_connection->get_id());
                iter = active_connections.erase_after(before_iter);

                // If we erased the last element, this check is essential
                if (iter == list.end()) {
                    break;
                }
            } else {
                client_connection->send(message);
            }
        }
    }

    void Server::check_connections() {
        const auto& list {active_connections};

        for (auto before_iter {list.before_begin()}, iter {list.begin()}; iter != list.end(); before_iter++, iter++) {
            const auto& client_connection = *iter;

            assert(client_connection != nullptr);

            if (!client_connection->is_open()) {
                on_client_disconnected(client_connection);
                clients.deallocate_id(client_connection->get_id());
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
                    if (stream) {
                        *stream << "Could not accept new connection: " << ec.message() << '\n';
                    }

                    if (!running) {
                        return;
                    }
                } else {
                    if (stream) {
                        *stream << "Accepted new connection: " << socket.remote_endpoint() << '\n';
                    }

                    const auto new_id {clients.allocate_id()};

                    if (!new_id) {
                        if (stream) {
                            *stream << "Actively rejected connection; pool is full\n";
                        }

                        socket.close();
                    } else {
                        create_new_connection(std::move(socket), *new_id);
                    }
                }

                task_accept_connection();
            }
        );
    }

    void Server::create_new_connection(asio::ip::tcp::socket&& socket, std::uint32_t id) {
        const auto connection {std::make_shared<ClientConnection>(
            &asio_context,
            std::move(socket),
            &incoming_messages,
            id
        )};

        if (on_client_connected(connection)) {
            active_connections.push_front(connection);
            connection->start_communication();

            if (stream) {
                *stream << "Approved connection [" << id << "]\n";
            }
        } else {
            connection->close();
            clients.deallocate_id(id);  // Take back the unused ID

            if (stream) {
                *stream << "Actively rejected connection from server side code\n";
            }
        }
    }
}
