#include "rain_net/internal/server_connection.hpp"

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

#include "rain_net/internal/error.hpp"
#include "rain_net/conversion.hpp"  // TODO

namespace rain_net {
    void ServerConnection::send(const Message& message) {
        task_send_message(message);
    }

    void ServerConnection::connect() {
        task_connect_to_server();
    }

    bool ServerConnection::connection_established() const noexcept {
        return established_connection.load();
    }

    void ServerConnection::add_to_incoming_messages() {
        incoming_messages.push_back(Message(current_incoming_message.header, std::move(current_incoming_message.payload)));

        current_incoming_message = {};
    }

    void ServerConnection::task_write_message() {
        assert(!outgoing_messages.empty());

        std::vector<asio::const_buffer> buffers;
        buffers.emplace_back(&outgoing_messages.front().header, sizeof(internal::MsgHeader));

        if (outgoing_messages.front().header.payload_size > 0) {
            buffers.emplace_back(outgoing_messages.front().payload.get(), outgoing_messages.front().header.payload_size);
        }

        const std::size_t size {internal::buffers_size(buffers)};

        asio::async_write(tcp_socket, buffers,
            [this, size](asio::error_code ec, [[maybe_unused]] std::size_t bytes_transferred) {
                if (ec) {
                    tcp_socket.close();

                    throw ConnectionError("Could not write message: " + ec.message());
                }

                assert(bytes_transferred == size);

                outgoing_messages.pop_front();

                // Thus writing tasks can stop
                if (!outgoing_messages.empty()) {
                    task_write_message();
                }
            }
        );
    }

    void ServerConnection::task_read_header() {
        asio::async_read(tcp_socket, asio::buffer(&current_incoming_message.header, sizeof(internal::MsgHeader)),
            [this](asio::error_code ec, [[maybe_unused]] std::size_t bytes_transferred) {
                if (ec) {
                    tcp_socket.close();

                    throw ConnectionError("Could not read header: " + ec.message());
                }

                assert(bytes_transferred == sizeof(internal::MsgHeader));

                // Check if there is a payload to read
                if (current_incoming_message.header.payload_size > 0) {
                    // Allocate space so that we write to it later
                    current_incoming_message.payload = std::make_unique<unsigned char[]>(current_incoming_message.header.payload_size);

                    task_read_payload();
                } else {
                    add_to_incoming_messages();
                    task_read_header();
                }
            }
        );
    }

    void ServerConnection::task_read_payload() {
        asio::async_read(tcp_socket, asio::buffer(current_incoming_message.payload.get(), current_incoming_message.header.payload_size),
            [this](asio::error_code ec, [[maybe_unused]] std::size_t bytes_transferred) {
                if (ec) {
                    tcp_socket.close();

                    throw ConnectionError("Could not read payload: " + ec.message());
                }

                assert(bytes_transferred == current_incoming_message.header.payload_size);

                add_to_incoming_messages();
                task_read_header();
            }
        );
    }

    void ServerConnection::task_send_message(const Message& message) {
        asio::post(asio_context,
            [this, message]() {
                const bool writing_tasks_stopped {outgoing_messages.empty()};

                outgoing_messages.push_back(internal::clone_message(message));

                // Restart the writing process, if it has stopped before
                if (writing_tasks_stopped) {
                    task_write_message();
                }
            }
        );
    }

    void ServerConnection::task_connect_to_server() {
        asio::async_connect(tcp_socket, endpoints,
            [this](asio::error_code ec, asio::ip::tcp::endpoint) {
                if (ec) {
                    tcp_socket.close();

                    throw ConnectionError("Could not connect to server: " + ec.message());
                }

                task_read_header();

                established_connection.store(true);
            }
        );
    }
}
