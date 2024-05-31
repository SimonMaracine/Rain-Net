#include "rain_net/internal/connection.hpp"

#include <iostream>
#include <cstddef>
#include <cassert>

#ifdef __GNUG__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wconversion"
#endif

#include <asio/buffer.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>
#include <asio/post.hpp>
#include <asio/connect.hpp>
#include <asio/error_code.hpp>

#ifdef __GNUG__
    #pragma GCC diagnostic pop
#endif

namespace rain_net {
    namespace internal {
        void Connection::close() {
            asio::post(*asio_context, [this]() {
                tcp_socket.close();  // FIXME throws exception
            });
        }

        bool Connection::is_open() const {
            return tcp_socket.is_open();
        }
    }

    void ClientConnection::send(const Message& message) {
        task_send_message(message);
    }

    void ClientConnection::start_communication() {
        task_read_header();
    }

    std::uint32_t ClientConnection::get_id() const {
        return client_id;
    }

    void ClientConnection::task_write_header() {
        assert(!outgoing_messages.empty());

        asio::async_write(tcp_socket, asio::buffer(&outgoing_messages.front().header, sizeof(internal::MsgHeader)),
            [this](asio::error_code ec, std::size_t size) {
                if (ec) {
                    std::cout << "Could not write header [" << get_id() << "]\n";  // TODO logging

                    tcp_socket.close();
                    return;
                }

                if (size < sizeof(internal::MsgHeader)) {
                    std::cout << "Could not write whole header [" << get_id() << "]\n";  // TODO logging

                    tcp_socket.close();
                    return;
                }

                // Check if there is a payload to write
                if (outgoing_messages.front().header.payload_size > 0) {
                    task_write_payload();
                } else {
                    outgoing_messages.pop_front();

                    // Thus writing tasks can stop
                    if (!outgoing_messages.empty()) {
                        task_write_header();
                    }
                }
            }
        );
    }

    void ClientConnection::task_write_payload() {
        assert(!outgoing_messages.empty());
        assert(outgoing_messages.front().header.payload_size == outgoing_messages.front().payload.size());

        asio::async_write(tcp_socket, asio::buffer(outgoing_messages.front().payload.data(), outgoing_messages.front().header.payload_size),
            [this](asio::error_code ec, std::size_t size) {
                if (ec) {
                    std::cout << "Could not write payload [" << get_id() << "]\n";  // TODO logging

                    tcp_socket.close();
                    return;
                }

                if (size < outgoing_messages.front().header.payload_size) {
                    std::cout << "Could not write whole payload [" << get_id() << "]\n";  // TODO logging

                    tcp_socket.close();
                    return;
                }

                outgoing_messages.pop_front();

                // Thus writing tasks can stop
                if (!outgoing_messages.empty()) {
                    task_write_header();
                }
            }
        );
    }

    void ClientConnection::task_read_header() {
        asio::async_read(tcp_socket, asio::buffer(&current_incoming_message.header, sizeof(internal::MsgHeader)),
            [this](asio::error_code ec, std::size_t size) {
                if (ec) {
                    std::cout << "Could not read header [" << get_id() << "]\n";  // TODO logging

                    tcp_socket.close();
                    return;
                }

                if (size < sizeof(internal::MsgHeader)) {
                    std::cout << "Could not read whole header [" << get_id() << "]\n";  // TODO logging

                    tcp_socket.close();
                    return;
                }

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
        );
    }

    void ClientConnection::task_read_payload() {
        assert(current_incoming_message.header.payload_size == current_incoming_message.payload.size());

        asio::async_read(tcp_socket, asio::buffer(current_incoming_message.payload.data(), current_incoming_message.header.payload_size),
            [this](asio::error_code ec, std::size_t size) {
                if (ec) {
                    std::cout << "Could not read payload [" << get_id() << "]\n";  // TODO logging

                    tcp_socket.close();
                    return;
                }

                if (size < current_incoming_message.header.payload_size) {
                    std::cout << "Could not read whole payload [" << get_id() << "]\n";  // TODO logging

                    tcp_socket.close();
                    return;
                }

                add_to_incoming_messages();
                task_read_header();
            }
        );
    }

    void ClientConnection::task_send_message(const Message& message) {
        asio::post(*asio_context,
            [this, message]() {
                const bool writing_tasks_stopped {outgoing_messages.empty()};

                outgoing_messages.push_back(message);

                // Restart the writing process, if it has stopped before
                if (writing_tasks_stopped) {
                    task_write_header();
                }
            }
        );
    }

    void ClientConnection::add_to_incoming_messages() {
        internal::OwnedMsg message;
        message.message = std::move(current_incoming_message);
        message.remote = shared_from_this();

        incoming_messages->push_back(std::move(message));

        current_incoming_message = {};
    }

    void ServerConnection::send(const Message& message) {
        if (!established_connection.load()) {  // TODO good?
            return;
        }

        task_send_message(message);
    }

    void ServerConnection::connect() {
        task_connect_to_server();
    }

    bool ServerConnection::is_connected() const {
        return established_connection.load();
    }

    void ServerConnection::task_write_header() {
        assert(!outgoing_messages.empty());

        asio::async_write(tcp_socket, asio::buffer(&outgoing_messages.front().header, sizeof(internal::MsgHeader)),
            [this](asio::error_code ec, std::size_t size) {
                if (ec) {
                    std::cout << "Could not write header\n";  // TODO logging

                    tcp_socket.close();
                    return;
                }

                if (size < sizeof(internal::MsgHeader)) {
                    std::cout << "Could not write whole header\n";  // TODO logging

                    tcp_socket.close();
                    return;
                }

                // Check if there is a payload to write
                if (outgoing_messages.front().header.payload_size > 0) {
                    task_write_payload();
                } else {
                    outgoing_messages.pop_front();

                    // Thus writing tasks can stop
                    if (!outgoing_messages.empty()) {
                        task_write_header();
                    }
                }
            }
        );
    }

    void ServerConnection::task_write_payload() {
        assert(!outgoing_messages.empty());
        assert(outgoing_messages.front().header.payload_size == outgoing_messages.front().payload.size());

        asio::async_write(tcp_socket, asio::buffer(outgoing_messages.front().payload.data(), outgoing_messages.front().header.payload_size),
            [this](asio::error_code ec, std::size_t size) {
                if (ec) {
                    std::cout << "Could not write payload\n";  // TODO logging

                    tcp_socket.close();
                    return;
                }

                if (size < outgoing_messages.front().header.payload_size) {
                    std::cout << "Could not write whole payload\n";  // TODO logging

                    tcp_socket.close();
                    return;
                }

                outgoing_messages.pop_front();

                // Thus writing tasks can stop
                if (!outgoing_messages.empty()) {
                    task_write_header();
                }
            }
        );
    }

    void ServerConnection::task_read_header() {
        asio::async_read(tcp_socket, asio::buffer(&current_incoming_message.header, sizeof(internal::MsgHeader)),
            [this](asio::error_code ec, std::size_t size) {
                if (ec) {
                    std::cout << "Could not read header\n";  // TODO logging

                    tcp_socket.close();
                    return;
                }

                if (size < sizeof(internal::MsgHeader)) {
                    std::cout << "Could not read whole header\n";  // TODO logging

                    tcp_socket.close();
                    return;
                }

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
        );
    }

    void ServerConnection::task_read_payload() {
        assert(current_incoming_message.header.payload_size == current_incoming_message.payload.size());

        asio::async_read(tcp_socket, asio::buffer(current_incoming_message.payload.data(), current_incoming_message.header.payload_size),
            [this](asio::error_code ec, std::size_t size) {
                if (ec) {
                    std::cout << "Could not read payload\n";  // TODO logging

                    tcp_socket.close();
                    return;
                }

                if (size < current_incoming_message.header.payload_size) {
                    std::cout << "Could not read whole payload\n";  // TODO logging

                    tcp_socket.close();
                    return;
                }

                add_to_incoming_messages();
                task_read_header();
            }
        );
    }

    void ServerConnection::task_send_message(const Message& message) {
        asio::post(*asio_context,
            [this, message]() {
                const bool writing_tasks_stopped {outgoing_messages.empty()};

                outgoing_messages.push_back(message);

                // Restart the writing process, if it has stopped before
                if (writing_tasks_stopped) {
                    task_write_header();
                }
            }
        );
    }

    void ServerConnection::task_connect_to_server() {
        asio::async_connect(tcp_socket, endpoints,
            [this](asio::error_code ec, asio::ip::tcp::endpoint endpoint) {
                if (ec) {
                    tcp_socket.close();
                    return;
                }

                task_read_header();

                established_connection.store(true);
            }
        );
    }

    void ServerConnection::add_to_incoming_messages() {
        incoming_messages->push_back(std::move(current_incoming_message));

        current_incoming_message = {};
    }
}
