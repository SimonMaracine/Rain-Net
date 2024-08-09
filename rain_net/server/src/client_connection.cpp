#include "rain_net/internal/client_connection.hpp"

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
    void ClientConnection::send(const Message& message) {
        task_send_message(message);
    }

    std::uint32_t ClientConnection::get_id() const noexcept {
        return client_id;
    }

    void ClientConnection::start_communication() {
        task_read_header();
    }

    void ClientConnection::add_to_incoming_messages() {
        incoming_messages.push_back(std::make_pair(
            Message(current_incoming_message.header, std::move(current_incoming_message.payload)),
            shared_from_this()
        ));

        current_incoming_message = {};
    }

    void ClientConnection::task_write_message() {
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

                    log('[' + std::to_string(get_id()) + "] Could not write message: " + ec.message());
                    return;
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

    void ClientConnection::task_read_header() {
        asio::async_read(tcp_socket, asio::buffer(&current_incoming_message.header, sizeof(internal::MsgHeader)),
            [this](asio::error_code ec, [[maybe_unused]] std::size_t bytes_transferred) {
                if (ec) {
                    tcp_socket.close();

                    log('[' + std::to_string(get_id()) + "] Could not read header: " + ec.message());
                    return;
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

    void ClientConnection::task_read_payload() {
        asio::async_read(tcp_socket, asio::buffer(current_incoming_message.payload.get(), current_incoming_message.header.payload_size),
            [this](asio::error_code ec, [[maybe_unused]] std::size_t bytes_transferred) {
                if (ec) {
                    tcp_socket.close();

                    log('[' + std::to_string(get_id()) + "] Could not read payload: " + ec.message());
                    return;
                }

                assert(bytes_transferred == current_incoming_message.header.payload_size);

                add_to_incoming_messages();
                task_read_header();
            }
        );
    }

    void ClientConnection::task_send_message(const Message& message) {
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
}
