#include <cstdint>
#include <memory>
#include <thread>
#include <deque>
#include <limits>
#include <utility>
#include <cassert>
#include <algorithm>
#include <iostream>

#define ASIO_NO_DEPRECATED
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

    void Server::start() {
        // Handle some work to the context before it closes automatically
        task_wait_for_connection();  // TODO error check

        context_thread = std::thread([this]() {
            asio_context.run();
        });

        std::cout << "Server started on port " << listen_port << '\n';  // TODO logging
    }

    void Server::stop() {
        asio_context.stop();
        context_thread.join();

        std::cout << "Server stopped\n";
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

        if (client_connection->is_connected()) {
            client_connection->send(message);
        } else {
            // Client has disconnected for any reason
            on_client_disconnected(client_connection);

            // Remove this specific client from the list
            active_connections.erase(
                std::remove(active_connections.begin(), active_connections.end(), client_connection),
                active_connections.cend()
            );
        }
    }

    void Server::send_message_all(const Message& message, std::shared_ptr<Connection> exception) {
        bool disconnected_clients = false;

        for (auto& client_connection : active_connections) {
            assert(client_connection != nullptr);

            if (client_connection == exception) {
                continue;
            }

            if (client_connection->is_connected()) {
                client_connection->send(message);
            } else {
                // Client has disconnected for any reason
                on_client_disconnected(client_connection);

                client_connection.reset();  // Destroy this client
                disconnected_clients = true;
            }
        }

        if (disconnected_clients) {
            // Remove all clients previously destroyed
            active_connections.erase(
                std::remove(active_connections.begin(), active_connections.end(), nullptr),
                active_connections.cend()
            );
        }
    }

    void Server::task_wait_for_connection() {
        acceptor.async_accept(
            [this](asio::error_code ec, asio::ip::tcp::socket socket) {
                if (ec) {
                    std::cout << "Could not accept a new connection: " << ec.message() << '\n';  // TODO logging
                } else {
                    std::cout << "Accepted a new connection " << socket.remote_endpoint() << '\n';

                    std::shared_ptr<Connection> new_connection = std::make_shared<internal::ClientConnection>(
                        &asio_context, &incoming_messages, std::move(socket), ++client_id_counter
                    );

                    if (on_client_connected(new_connection)) {
                        active_connections.push_back(std::move(new_connection));

                        active_connections.back()->try_connect();

                        std::cout << "Approved connection " << active_connections.back()->get_id() << '\n';  // TODO logging
                    } else {
                        client_id_counter--;  // Take back the unused id

                        std::cout << "Actively rejected connection\n";
                    }
                }

                task_wait_for_connection();
            }
        );
    }
}
