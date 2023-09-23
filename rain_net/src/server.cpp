#include <cstdint>
#include <memory>
#include <thread>
#include <forward_list>
#include <limits>
#include <optional>
#include <utility>
#include <cassert>
#include <algorithm>
#include <iostream>

#include <asio/io_context.hpp>
#include <asio/error_code.hpp>
#include <asio/ip/tcp.hpp>

#include "rain_net/server.hpp"
#include "rain_net/queue.hpp"
#include "rain_net/message.hpp"
#include "rain_net/connection.hpp"

namespace rain_net {
    Server::~Server() {
        stop();
    }

    void Server::start(std::uint32_t max_clients) {
        clients.create_pool(max_clients);

        // Handle some work to the context before it closes automatically
        task_wait_for_connection();  // TODO error check

        context_thread = std::thread([this]() {
            asio_context.run();
        });

        std::cout << "Server started on port " << listen_port << '\n';  // TODO logging

        stoppable = true;
    }

    void Server::stop() {
        if (!stoppable) {
            return;
        }

        asio_context.stop();
        context_thread.join();

        std::cout << "Server stopped\n";

        stoppable = false;
    }

    void Server::update(const std::uint32_t max_messages, bool wait) {
        if (wait) {
            incoming_messages.wait();
        }

        std::uint32_t messages_processed = 0;

        while (messages_processed < max_messages && !incoming_messages.empty()) {
            internal::OwnedMsg owned_msg = incoming_messages.pop_front();

            assert(owned_msg.remote != nullptr);

            on_message_received(owned_msg.remote, owned_msg.message);

            messages_processed++;
        }
    }

    void Server::send_message(std::shared_ptr<Connection> client_connection, const Message& message) {
        assert(client_connection != nullptr);

        if (client_connection->is_open()) {
            client_connection->send(message);
        } else {
            // Client has disconnected for any reason
            on_client_disconnected(client_connection);

            // Get back the ID
            clients.deallocate_id(client_connection->get_id());

            // Remove this specific client from the list
            active_connections.remove(client_connection);
        }
    }

    void Server::send_message_all(const Message& message, std::shared_ptr<Connection> exception) {
        const auto& list = active_connections;

        for (auto before_iter = list.before_begin(), iter = list.begin(); iter != list.end(); before_iter++, iter++) {
            auto& client_connection = *iter;

            assert(client_connection != nullptr);

            if (client_connection == exception) {
                continue;
            }

            if (client_connection->is_open()) {
                client_connection->send(message);
            } else {
                // Client has disconnected for any reason
                on_client_disconnected(client_connection);

                // Get back the ID
                clients.deallocate_id(client_connection->get_id());

                // Delete this client
                iter = active_connections.erase_after(before_iter);
            }
        }
    }

    void Server::check_connections() {
        const auto& list = active_connections;

        for (auto before_iter = list.before_begin(), iter = list.begin(); iter != list.end(); before_iter++, iter++) {
            auto& client_connection = *iter;

            assert(client_connection != nullptr);

            if (!client_connection->is_open()) {
                // Client has disconnected for any reason
                on_client_disconnected(client_connection);

                // Get back the ID
                clients.deallocate_id(client_connection->get_id());

                // Delete this client
                iter = active_connections.erase_after(before_iter);
            }
        }
    }

    void Server::task_wait_for_connection() {
        acceptor.async_accept(
            [this](asio::error_code ec, asio::ip::tcp::socket socket) {
                if (ec) {
                    std::cout << "Could not accept a new connection: " << ec.message() << '\n';  // TODO logging
                } else {
                    std::cout << "Accepted a new connection: " << socket.remote_endpoint() << '\n';

                    const auto id = clients.allocate_id();

                    if (id.has_value()) {
                        create_new_connection(std::move(socket), *id);
                    } else {
                        std::cout << "Actively rejected connection, as the server is fully occupied\n";
                    }
                }

                task_wait_for_connection();
            }
        );
    }

    void Server::create_new_connection(asio::ip::tcp::socket&& socket, std::uint32_t id) {
        auto connection = std::make_shared<internal::ClientConnection>(
            &asio_context,
            &incoming_messages,
            std::move(socket),
            id
        );

        if (on_client_connected(connection)) {
            active_connections.push_front(std::move(connection));

            active_connections.front()->try_connect();

            std::cout << "Approved connection with ID " << id << '\n';  // TODO logging
        } else {
            clients.deallocate_id(id);  // Take back the unused ID

            std::cout << "Actively rejected connection from server side code\n";
        }
    }

    void Server::ClientsPool::create_pool(std::uint32_t size) {
        pool = new bool[size];
        this->size = size;
    }

    std::optional<std::uint32_t> Server::ClientsPool::allocate_id() {
        const auto result = search_id(id_pointer, size);

        if (result != std::nullopt) {
            return result;
        }

        // No ID found; start searching from the beginning

        return search_id(0, id_pointer);

        // Returns ID or null, if really nothing found
    }

    void Server::ClientsPool::deallocate_id(std::uint32_t id) {
        pool[id] = false;
    }

    std::optional<std::uint32_t> Server::ClientsPool::search_id(std::uint32_t begin, std::uint32_t end) {
        for (std::uint32_t id = begin; id < end; id++) {
            if (!pool[id]) {
                pool[id] = true;
                id_pointer = (id + 1) % size;

                return std::make_optional(id);
            }
        }

        return std::nullopt;
    }
}
