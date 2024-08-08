#pragma once

#include <utility>
#include <atomic>

#include "rain_net/internal/connection.hpp"

namespace rain_net {
    class Client;

    // Owner of this is the client
    class ServerConnection final : public internal::Connection {
    public:
        ServerConnection(
            asio::io_context& asio_context,
            asio::ip::tcp::socket&& tcp_socket,
            internal::SyncQueue<Message>& incoming_messages,
            const asio::ip::tcp::resolver::results_type& endpoints
        )
            : internal::Connection(asio_context, std::move(tcp_socket)), incoming_messages(incoming_messages),
            endpoints(endpoints) {}

        // Send a message asynchronously
        void send(const Message& message);
    private:
        void connect();
        bool connection_established() const noexcept;
        void add_to_incoming_messages();

        void task_write_message();
        void task_read_header();
        void task_read_payload();
        void task_send_message(const Message& message);
        void task_connect_to_server();

        internal::SyncQueue<Message>& incoming_messages;
        std::atomic_bool established_connection {false};
        asio::ip::tcp::resolver::results_type endpoints;

        friend class Client;
    };
}
