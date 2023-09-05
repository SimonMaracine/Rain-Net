#include <utility>
#include <memory>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstddef>

#define ASIO_NO_DEPRECATED
#include <asio/buffer.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>
#include <asio/post.hpp>
#include <asio/connect.hpp>
#include <asio/error_code.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

#include "rain_net/connection.hpp"
#include "rain_net/message.hpp"
#include "rain_net/queue.hpp"

namespace rain_net {
    std::uint32_t Connection::get_id() const {
        return 0;  // This means that it's a connection to the server
    }

    void Connection::disconnect() {
        if (!tcp_socket.is_open()) {
            return;
        }

        task_close_socket();
    }

    bool Connection::is_connected() const {
        return tcp_socket.is_open();
    }

    void Connection::send(const Message& message) {
        task_try_send_message(message);
    }

    void Connection::close_connection_on_this_side() {
        // Close the connection on this side; the remote will pick this up
        tcp_socket.close();
    }

    void Connection::task_read_header() {
        static_assert(std::is_trivially_copyable_v<internal::MsgHeader>);

        asio::async_read(tcp_socket, asio::buffer(&current_incoming_message.header, sizeof(internal::MsgHeader)),
            [this](asio::error_code ec, [[maybe_unused]] std::size_t size) {
                if (ec) {
                    std::cout << "Could not read header [" << get_id() <<  "]\n";  // TODO logging

                    close_connection_on_this_side();
                } else {
                    assert(size == sizeof(internal::MsgHeader));

                    // Check if there is a payload to read
                    if (current_incoming_message.header.payload_size > 0) {
                        // Allocate space so that we write to it later
                        current_incoming_message.payload.resize(current_incoming_message.header.payload_size);

                        task_read_payload();
                    } else {
                        add_to_incoming_messages();
                        task_read_header();
                    }
                }
            }
        );
    }

    void Connection::task_read_payload() {
        assert(current_incoming_message.payload.size() == current_incoming_message.header.payload_size);

        asio::async_read(tcp_socket, asio::buffer(current_incoming_message.payload.data(), current_incoming_message.header.payload_size),
            [this](asio::error_code ec, [[maybe_unused]] std::size_t size) {
                if (ec) {
                    std::cout << "Could not read payload [" << get_id() << "]\n";

                    close_connection_on_this_side();
                } else {
                    assert(size == current_incoming_message.header.payload_size);

                    add_to_incoming_messages();
                    task_read_header();
                }
            }
        );
    }

    void Connection::task_write_header() {
        static_assert(std::is_trivially_copyable_v<internal::MsgHeader>);
        assert(!outgoing_messages.empty());

        asio::async_write(tcp_socket, asio::buffer(&outgoing_messages.front().header, sizeof(internal::MsgHeader)),
            [this](asio::error_code ec, [[maybe_unused]] std::size_t size) {
                if (ec) {
                    std::cout << "Could not write header [" << get_id() << "]\n";  // TODO logging

                    close_connection_on_this_side();
                } else {
                    assert(size == sizeof(internal::MsgHeader));

                    // Check if there is a payload to write
                    if (outgoing_messages.front().header.payload_size > 0) {
                        task_write_payload();
                    } else {
                        // Finish with this message
                        outgoing_messages.pop_front();

                        if (!outgoing_messages.empty()) {  // Thus writing tasks can stop
                            task_write_header();
                        }
                    }
                }
            }
        );
    }

    void Connection::task_write_payload() {
        assert(!outgoing_messages.empty());
        assert(outgoing_messages.front().payload.size() == outgoing_messages.front().header.payload_size);

        asio::async_write(tcp_socket, asio::buffer(outgoing_messages.front().payload.data(), outgoing_messages.front().header.payload_size),
            [this](asio::error_code ec, [[maybe_unused]] std::size_t size) {
                if (ec) {
                    std::cout << "Could not write payload [" << get_id() << "]\n";

                    close_connection_on_this_side();
                } else {
                    assert(size == outgoing_messages.front().header.payload_size);

                    // Finish with this message
                    outgoing_messages.pop_front();

                    if (!outgoing_messages.empty()) {  // Thus writing tasks can stop
                        task_write_header();
                    }
                }
            }
        );
    }

    void Connection::task_try_send_message(const Message& message) {  // TODO this is slow
        asio::post(*asio_context,
            [this, message]() {
                if (!established_connection.load()) {
                    task_try_send_message(message);
                } else {
                    task_send_message(message);
                }
            }
        );
    }

    void Connection::task_send_message(const Message& message) {
        // This really needs to be a task
        asio::post(*asio_context,
            [this, message]() {
                const bool writing_tasks_stopped = outgoing_messages.empty();

                outgoing_messages.push_back(message);

                // Restart the writing process, if it has stopped before
                if (writing_tasks_stopped) {
                    task_write_header();
                }
            }
        );
    }

    void Connection::task_close_socket() {
        asio::post(*asio_context,
            [this]() {
                tcp_socket.close();
            }
        );
    }

    namespace internal {
        ClientConnection::ClientConnection(asio::io_context* asio_context, Queue<OwnedMsg>* incoming_messages, asio::ip::tcp::socket&& tcp_socket, std::uint32_t client_id)
            : Connection(asio_context, incoming_messages, std::move(tcp_socket)), client_id(client_id) {
            // The connection has established before
            established_connection.store(true);
        }

        void ClientConnection::try_connect() {  // Connect to client
            std::cout << "Connecting to client...\n";  // TODO logging

            task_read_header();
        }

        std::uint32_t ClientConnection::get_id() const {
            return client_id;
        }

        void ClientConnection::add_to_incoming_messages() {
            OwnedMsg owned_message;
            owned_message.message = current_incoming_message;
            owned_message.remote = shared_from_this();  // This distinction is important

            incoming_messages->push_back(std::move(owned_message));

            current_incoming_message = {};
        }

        void ServerConnection::try_connect() {  // Connect to server
            std::cout << "Trying to connect to server...\n";  // TODO logging

            task_connect_to_server();
        }

        void ServerConnection::add_to_incoming_messages() {
            OwnedMsg owned_message;
            owned_message.message = current_incoming_message;
            owned_message.remote = nullptr;  // This distinction is important

            incoming_messages->push_back(std::move(owned_message));

            current_incoming_message = {};
        }

        void ServerConnection::task_connect_to_server() {
            asio::async_connect(tcp_socket, endpoints,
                [this](asio::error_code ec, asio::ip::tcp::endpoint endpoint) {
                    if (ec) {
                        std::cout << "Could not connect to server\n";  // TODO logging

                        close_connection_on_this_side();  // TODO need?

                        return;
                    } else {
                        std::cout << "Successfully connected to " << endpoint << '\n';

                        task_read_header();

                        // Now messages can be sent
                        established_connection.store(true);
                    }
                }
            );
        }
    }
}
