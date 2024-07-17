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
    class Client;
    class Server;

    namespace internal {
        class Connection {
        protected:
            ~Connection() = default;

            Connection(const Connection&) = delete;
            Connection& operator=(const Connection&) = delete;
            Connection(Connection&&) = delete;
            Connection& operator=(Connection&&) = delete;
        protected:
            // Close the connection asynchronously
            void close();

            // Check if the socket is open
            bool is_open() const;

            Connection(asio::io_context& asio_context, asio::ip::tcp::socket&& tcp_socket)
                : asio_context(asio_context), tcp_socket(std::move(tcp_socket)) {}

            asio::io_context& asio_context;
            asio::ip::tcp::socket tcp_socket;

            internal::SyncQueue<Message> outgoing_messages;
            Message current_incoming_message;
        };
    }

    // Owner of this is the server
    class ClientConnection final : public internal::Connection, public std::enable_shared_from_this<ClientConnection> {
    public:
        ClientConnection(
            asio::io_context& asio_context,
            asio::ip::tcp::socket&& tcp_socket,
            internal::SyncQueue<internal::OwnedMsg>& incoming_messages,
            std::uint32_t client_id,
            const std::function<void(std::string&&)>& log_fn
        )
            : Connection(asio_context, std::move(tcp_socket)), incoming_messages(incoming_messages),
            log_fn(log_fn), client_id(client_id) {}

        // Send a message asynchronously
        void send(const Message& message);

        std::uint32_t get_id() const noexcept;
    private:
        void start_communication();
        void add_to_incoming_messages();

        void task_write_header();
        void task_write_payload();
        void task_read_header();
        void task_read_payload();
        void task_send_message(const Message& message);

        internal::SyncQueue<internal::OwnedMsg>& incoming_messages;
        const std::function<void(std::string&&)>& log_fn;
        std::uint32_t client_id {};  // Given by the server

        friend class Server;
    };

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

        void task_write_header();
        void task_write_payload();
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
