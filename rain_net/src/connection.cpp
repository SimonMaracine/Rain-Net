#include "rain_net/internal/connection.hpp"

#include <cstddef>
#include <string>
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

#include "rain_net/internal/error.hpp"

namespace rain_net {
    namespace internal {
        void Connection::close() {
            asio::post(asio_context, [this]() {
                if (!tcp_socket.is_open()) {
                    return;
                }

                tcp_socket.close();
            });
        }

        bool Connection::is_open() const {
            return tcp_socket.is_open();
        }
    }

    void ClientConnection::send(const Message& message) {
        task_send_message(message);
    }

    std::uint32_t ClientConnection::get_id() const {
        return client_id;
    }

    void ClientConnection::start_communication() {
        task_read_header();
    }

    void ClientConnection::add_to_incoming_messages() {
        internal::OwnedMsg message;
        message.message = std::move(current_incoming_message);
        message.remote = shared_from_this();

        incoming_messages.push_back(std::move(message));

        current_incoming_message = {};
    }

    void ClientConnection::task_write_header() {
        assert(!outgoing_messages.empty());

        asio::async_write(tcp_socket, asio::buffer(&outgoing_messages.front().header, sizeof(internal::MsgHeader)),
            [this](asio::error_code ec, std::size_t size) {
                if (ec) {
                    log_fn("[" + std::to_string(get_id()) + "] Could not write header: " + ec.message());

                    tcp_socket.close();
                    return;
                }

                if (size < sizeof(internal::MsgHeader)) {
                    log_fn("[" + std::to_string(get_id()) + "] Could not write whole header");

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

        asio::async_write(tcp_socket, asio::buffer(outgoing_messages.front().payload, outgoing_messages.front().header.payload_size),
            [this](asio::error_code ec, std::size_t size) {
                if (ec) {
                    log_fn("[" + std::to_string(get_id()) + "] Could not write payload: " + ec.message());

                    tcp_socket.close();
                    return;
                }

                if (size < outgoing_messages.front().header.payload_size) {
                    log_fn("[" + std::to_string(get_id()) + "] Could not write whole payload");

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
                    log_fn("[" + std::to_string(get_id()) + "] Could not read header: " + ec.message());

                    tcp_socket.close();
                    return;
                }

                if (size < sizeof(internal::MsgHeader)) {
                    log_fn("[" + std::to_string(get_id()) + "] Could not read whole header");

                    tcp_socket.close();
                    return;
                }

                // Check if there is a payload to read
                if (current_incoming_message.header.payload_size > 0) {
                    // Allocate space so that we write to it later
                    current_incoming_message.allocate(current_incoming_message.header.payload_size);

                    task_read_payload();
                } else {
                    add_to_incoming_messages();
                    task_read_header();
                }
            }
        );
    }

    void ClientConnection::task_read_payload() {
        asio::async_read(tcp_socket, asio::buffer(current_incoming_message.payload, current_incoming_message.header.payload_size),
            [this](asio::error_code ec, std::size_t size) {
                if (ec) {
                    log_fn("[" + std::to_string(get_id()) + "] Could not read payload: " + ec.message());

                    tcp_socket.close();
                    return;
                }

                if (size < current_incoming_message.header.payload_size) {
                    log_fn("[" + std::to_string(get_id()) + "] Could not read whole payload");

                    tcp_socket.close();
                    return;
                }

                add_to_incoming_messages();
                task_read_header();
            }
        );
    }

    void ClientConnection::task_send_message(const Message& message) {
        asio::post(asio_context,
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

    void ServerConnection::send(const Message& message) {
        task_send_message(message);
    }

    void ServerConnection::connect() {
        task_connect_to_server();
    }

    bool ServerConnection::connection_established() const {
        return established_connection.load();
    }

    void ServerConnection::add_to_incoming_messages() {
        incoming_messages.push_back(std::move(current_incoming_message));

        current_incoming_message = {};
    }

    void ServerConnection::task_write_header() {
        assert(!outgoing_messages.empty());

        asio::async_write(tcp_socket, asio::buffer(&outgoing_messages.front().header, sizeof(internal::MsgHeader)),
            [this](asio::error_code ec, std::size_t size) {
                if (ec) {
                    tcp_socket.close();

                    throw internal::ConnectionError("Could not write header: " + ec.message());
                }

                if (size < sizeof(internal::MsgHeader)) {
                    tcp_socket.close();

                    throw internal::ConnectionError("Could not write whole header");
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

        asio::async_write(tcp_socket, asio::buffer(outgoing_messages.front().payload, outgoing_messages.front().header.payload_size),
            [this](asio::error_code ec, std::size_t size) {
                if (ec) {
                    tcp_socket.close();

                    throw internal::ConnectionError("Could not write payload: " + ec.message());
                }

                if (size < outgoing_messages.front().header.payload_size) {
                    tcp_socket.close();

                    throw internal::ConnectionError("Could not write whole payload");
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
                    tcp_socket.close();

                    throw internal::ConnectionError("Could not read header: " + ec.message());
                }

                if (size < sizeof(internal::MsgHeader)) {
                    tcp_socket.close();

                    throw internal::ConnectionError("Could not read whole header");
                }

                // Check if there is a payload to read
                if (current_incoming_message.header.payload_size > 0) {
                    // Allocate space so that we write to it later
                    current_incoming_message.allocate(current_incoming_message.header.payload_size);

                    task_read_payload();
                } else {
                    add_to_incoming_messages();
                    task_read_header();
                }
            }
        );
    }

    void ServerConnection::task_read_payload() {
        asio::async_read(tcp_socket, asio::buffer(current_incoming_message.payload, current_incoming_message.header.payload_size),
            [this](asio::error_code ec, std::size_t size) {
                if (ec) {
                    tcp_socket.close();

                    throw internal::ConnectionError("Could not read payload: " + ec.message());
                }

                if (size < current_incoming_message.header.payload_size) {
                    tcp_socket.close();

                    throw internal::ConnectionError("Could not read whole payload");
                }

                add_to_incoming_messages();
                task_read_header();
            }
        );
    }

    void ServerConnection::task_send_message(const Message& message) {
        asio::post(asio_context,
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
            [this](asio::error_code ec, asio::ip::tcp::endpoint) {
                if (ec) {
                    tcp_socket.close();

                    throw internal::ConnectionError("Could not connect to server: " + ec.message());
                }

                task_read_header();

                established_connection.store(true);
            }
        );
    }
}
