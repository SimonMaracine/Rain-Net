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
            ~Connection() = default;  // FIXME hmm

            Connection(const Connection&) = delete;
            Connection& operator=(const Connection&) = delete;
            Connection(Connection&&) = delete;
            Connection& operator=(Connection&&) = delete;

            // Close the connection and check its status
            void close();
            bool is_open() const;

            // Low level send message routine; you shouldn't really use this
            void send(const Message& message);
        protected:
            Connection(asio::io_context* asio_context, asio::ip::tcp::socket&& tcp_socket)
                : asio_context(asio_context), tcp_socket(std::move(tcp_socket)) {}

            // These must be virtual and it's okay
            virtual void add_to_incoming_messages() = 0;
            virtual std::uint32_t get_id() const = 0;

            void task_read_header();
            void task_read_payload();
            void task_write_header();
            void task_write_payload();
            void task_try_send_message(const Message& message);
            void task_send_message(const Message& message);
            void task_close_socket();

            asio::io_context* asio_context {nullptr};

            asio::ip::tcp::socket tcp_socket;
            internal::SyncQueue<Message> outgoing_messages;

            Message current_incoming_message;

            // Set to true only once at the beginning
            std::atomic_bool established_connection {false};
        };
    }

    // Owner of this is the server
    class ClientConnection final : public internal::Connection, public std::enable_shared_from_this<ClientConnection> {
    public:
        ClientConnection(
            asio::io_context* asio_context,
            asio::ip::tcp::socket&& tcp_socket,
            internal::WaitingSyncQueue<internal::OwnedMsg<ClientConnection>>* incoming_messages,
            std::uint32_t client_id
        );

        void start_communication();
        std::uint32_t get_id() const override;
    private:
        void add_to_incoming_messages() override;

        internal::WaitingSyncQueue<internal::OwnedMsg<ClientConnection>>* incoming_messages {nullptr};

        std::uint32_t client_id {};  // This is given by the server
    };

    // Owner of this is the client
    class ServerConnection final : public internal::Connection {
    public:
        ServerConnection(
            asio::io_context* asio_context,
            asio::ip::tcp::socket&& tcp_socket,
            internal::SyncQueue<internal::OwnedMsg<ServerConnection>>* incoming_messages,
            const asio::ip::tcp::resolver::results_type& endpoints,
            const std::function<void()>& on_connected
        )
            : internal::Connection(asio_context, std::move(tcp_socket)), incoming_messages(incoming_messages),
            endpoints(endpoints), on_connected(on_connected) {}

        void connect();
    private:
        void add_to_incoming_messages() override;
        std::uint32_t get_id() const override;
        void task_connect_to_server();

        internal::SyncQueue<internal::OwnedMsg<ServerConnection>>* incoming_messages {nullptr};

        asio::ip::tcp::resolver::results_type endpoints;
        std::function<void()> on_connected;
    };
}
