#pragma once

#include <cstdint>
#include <memory>
#include <thread>
#include <utility>
#include <deque>
#include <cassert>
#include <algorithm>
#include <limits>

#include <asio.hpp>
#include <asio/ts/internet.hpp>

#include "queue.hpp"
#include "message.hpp"
#include "connection.hpp"

namespace rain_net {
    template<typename E>
    class Server {
    public:
        Server(uint16_t port)
            : acceptor(asio_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {

        }

        virtual ~Server() {
            stop();
        }

        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        Server(Server&&) = delete;
        Server& operator=(Server&&) = delete;

        bool start() {
            // Handle some work to the context before it closes automatically
            task_wait_for_connection();  // TODO error check

            context_thread = std::thread([this]() {
                asio_context.run();
            });

            std::cout << "Server started\n";  // TODO logging

            return true;
        }

        bool stop() {
            asio_context.stop();
            context_thread.join();

            std::cout << "Server stopped\n";  // TODO logging

            return true;
        }

        void update(const uint32_t max_messages = std::numeric_limits<uint32_t>::max()) {
            uint32_t messages_processed = 0;

            while (messages_processed < max_messages && !incoming_messages.empty()) {
                OwnedMessage<E> message = incoming_messages.pop_front();

                on_message(message.remote, message.msg);

                messages_processed++;
            }
        }

        void task_wait_for_connection() {
            acceptor.async_accept(
                [this](asio::error_code ec, asio::ip::tcp::socket socket) {
                    if (ec) {
                        std::cout << "Could not accept new connection: " << ec.message() << '\n';  // TODO logging
                    } else {
                        std::cout << "Accepted new connection " << socket.remote_endpoint() << '\n';

                        std::shared_ptr<Connection<E>> new_connection = std::make_shared<ClientConnection<E>>(
                            &asio_context, &incoming_messages, std::move(socket), client_id_counter++
                        );

                        if (on_client_connected(new_connection)) {
                            new_connection->connect();
                            active_connections.push_back(std::move(new_connection));

                            std::cout << "Approved connection " << active_connections.back().id() << '\n';  // TODO logging
                        } else {
                            std::cout << "Actively rejected connection\n";
                            client_id_counter--;  // Take back the unused id
                        }
                    }

                    task_wait_for_connection();
                }
            );
        }

        void message_client(std::shared_ptr<Connection<E>> client_connection, const Message<E>& message) {
            assert(client_connection != nullptr);  // TODO maybe just check

            if (client_connection->is_connected()) {
                client_connection->send(message);
            } else {
                // Client has surely disconnected for any reason
                on_client_disconnected(client_connection);

                // Remove this specific client from the list
                active_connections.erase(
                    std::remove(active_connections.cbegin(), active_connections.cend(), client_connection),
                    active_connections.cend()
                );
            }
        }

        void message_all_clients(const Message<E>& message, std::shared_ptr<Connection<E>> except = nullptr) {
            bool disconnected_clients = false;

            for (auto& client_connection : active_connections) {
                assert(client_connection != nullptr);  // TODO maybe just check

                if (client_connection->is_connected()) {
                    if (client_connection != except) {
                        client_connection->send(message);
                    }
                } else {
                    // Client has surely disconnected for any reason
                    on_client_disconnected(client_connection);

                    client_connection.reset();  // Destroy this client
                    disconnected_clients = true;
                }
            }

            if (disconnected_clients) {
                // Remove all clients previously destroyed
                active_connections.erase(
                    std::remove(active_connections.cbegin(), active_connections.cend(), nullptr),
                    active_connections.cend()
                );
            }
        }
    protected:
        // Return false to reject the client, true otherwise
        virtual bool on_client_connected(std::shared_ptr<Connection<E>> client_connection) = 0;

        virtual void on_client_disconnected(std::shared_ptr<Connection<E>> client_connection) = 0;

        virtual void on_message(std::shared_ptr<Connection<E>> client_connection, Message<E>& message) = 0;

        Queue<OwnedMessage<E>> incoming_messages;
        std::deque<std::shared_ptr<Connection<E>>> active_connections;  // TODO protected? deque?
    private:
        asio::io_context asio_context;
        std::thread context_thread;

        asio::ip::tcp::acceptor acceptor;

        uint32_t client_id_counter = 0;
    };
}
