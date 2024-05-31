#pragma once

#include <utility>
#include <memory>
#include <atomic>
#include <cstdint>
#include <functional>

#ifdef __GNUG__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wconversion"
#endif

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

#ifdef __GNUG__
    #pragma GCC diagnostic pop
#endif

#include "rain_net/internal/message.hpp"
#include "rain_net/internal/queue.hpp"

namespace rain_net {
    namespace internal {
        class Connection {
        public:
            ~Connection() = default;

            Connection(const Connection&) = delete;
            Connection& operator=(const Connection&) = delete;
            Connection(Connection&&) = delete;
            Connection& operator=(Connection&&) = delete;

            // Close the connection asynchronously
            void close();

            // Check connection status
            bool is_open() const;
        protected:
            Connection(asio::io_context* asio_context, asio::ip::tcp::socket&& tcp_socket)
                : asio_context(asio_context), tcp_socket(std::move(tcp_socket)) {}

            asio::io_context* asio_context {nullptr};
            asio::ip::tcp::socket tcp_socket;

            internal::SyncQueue<Message> outgoing_messages;

            Message current_incoming_message;
        };
    }

    // Owner of this is the server
    class ClientConnection final : public internal::Connection, public std::enable_shared_from_this<ClientConnection> {
    public:
        ClientConnection(
            asio::io_context* asio_context,
            asio::ip::tcp::socket&& tcp_socket,
            internal::SyncQueue<internal::OwnedMsg>* incoming_messages,
            std::uint32_t client_id
        )
            : Connection(asio_context, std::move(tcp_socket)), incoming_messages(incoming_messages), client_id(client_id) {}

        // Send the message asynchronously
        void send(const Message& message);

        void start_communication();
        std::uint32_t get_id() const;
    private:
        void task_write_header();
        void task_write_payload();
        void task_read_header();
        void task_read_payload();
        void task_send_message(const Message& message);

        void add_to_incoming_messages();

        internal::SyncQueue<internal::OwnedMsg>* incoming_messages {nullptr};

        std::uint32_t client_id {};  // Given by the server
    };

    // Owner of this is the client
    class ServerConnection final : public internal::Connection {
    public:
        ServerConnection(
            asio::io_context* asio_context,
            asio::ip::tcp::socket&& tcp_socket,
            internal::SyncQueue<Message>* incoming_messages,
            const asio::ip::tcp::resolver::results_type& endpoints
        )
            : internal::Connection(asio_context, std::move(tcp_socket)), incoming_messages(incoming_messages),
            endpoints(endpoints) {}

        // Send the message asynchronously
        void send(const Message& message);

        void connect();
        bool is_connected() const;
    private:
        void task_write_header();
        void task_write_payload();
        void task_read_header();
        void task_read_payload();
        void task_send_message(const Message& message);
        void task_connect_to_server();

        void add_to_incoming_messages();

        internal::SyncQueue<Message>* incoming_messages {nullptr};

        std::atomic_bool established_connection {false};

        asio::ip::tcp::resolver::results_type endpoints;
    };
}
