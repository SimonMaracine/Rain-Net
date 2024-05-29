#include "rain_net/server.hpp"

#include <iostream>
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

        PoolClients::~PoolClients() {
            destroy_pool();
        }

        PoolClients::PoolClients(PoolClients&& other) noexcept
            : pool(std::exchange(other.pool, nullptr)), id_pointer(other.id_pointer), size(other.size) {}

        PoolClients& PoolClients::operator=(PoolClients&& other) noexcept {
            delete[] pool;

            pool = std::exchange(other.pool, nullptr);
            id_pointer = other.id_pointer;
            size = other.size;

            return *this;
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
            pool = new bool[size];
            this->size = size;
        }

        void PoolClients::destroy_pool() {
            delete[] pool;
            pool = nullptr;
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

        // Handle some work to the context before it closes automatically
        task_accept_connection();  // TODO error check

        context_thread = std::thread([this]() {
            asio_context.run();
        });

        std::cout << "Server started\n";  // TODO logging
    }

    void Server::stop() {
        asio_context.stop();
        context_thread.join();

        std::cout << "Server stopped\n";
    }

    void Server::update(std::uint32_t max_messages, bool wait) {
        if (wait) {
            incoming_messages.wait();
        }

        std::uint32_t messages_processed {0};

        while (messages_processed < max_messages && !incoming_messages.empty()) {
            const auto owned_msg {incoming_messages.pop_front()};

            assert(owned_msg.remote != nullptr);

            on_message_received(owned_msg.remote, owned_msg.message);

            messages_processed++;
        }
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

    void Server::send_message_all(const Message& message, std::shared_ptr<ClientConnection> exception) {
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
                    std::cout << "Could not accept a new connection: " << ec.message() << '\n';  // TODO logging
                } else {
                    std::cout << "Accepted a new connection: " << socket.remote_endpoint() << '\n';

                    const auto new_id {clients.allocate_id()};

                    if (!new_id) {
                        std::cout << "Actively rejected connection, as the server is fully occupied\n";

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

            std::cout << "Approved connection with ID " << id << '\n';  // TODO logging
        } else {
            connection->close();
            clients.deallocate_id(id);  // Take back the unused ID

            std::cout << "Actively rejected connection from server side code\n";
        }
    }
}
