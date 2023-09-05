#pragma once

#include <utility>
#include <memory>
#include <atomic>
#include <cstdint>

#define ASIO_NO_DEPRECATED
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

#include "rain_net/message.hpp"
#include "rain_net/queue.hpp"

namespace rain_net {
    class Connection {
    public:
        Connection(asio::io_context* asio_context, internal::Queue<internal::OwnedMsg>* incoming_messages, asio::ip::tcp::socket&& tcp_socket)
            : asio_context(asio_context), incoming_messages(incoming_messages), tcp_socket(std::move(tcp_socket)) {}

        virtual ~Connection() noexcept = default;  // FIXME hmm

        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;
        Connection(Connection&&) = delete;
        Connection& operator=(Connection&&) = delete;

        virtual void try_connect() = 0;
        virtual std::uint32_t get_id() const;

        void disconnect();
        bool is_connected() const;
        void send(const Message& message);
    protected:
        virtual void add_to_incoming_messages() = 0;

        void close_connection_on_this_side();
        void task_read_header();
        void task_read_payload();
        void task_write_header();
        void task_write_payload();
        void task_try_send_message(const Message& message);
        void task_send_message(const Message& message);
        void task_close_socket();

        asio::io_context* asio_context = nullptr;
        internal::Queue<internal::OwnedMsg>* incoming_messages = nullptr;

        asio::ip::tcp::socket tcp_socket;
        internal::Queue<Message> outgoing_messages;

        Message current_incoming_message;

        // Set to true only once at the beginning
        std::atomic<bool> established_connection = false;
    };

    namespace internal {
        // Owner of this is the server
        class ClientConnection final : public Connection, public std::enable_shared_from_this<ClientConnection> {
        public:
            ClientConnection(asio::io_context* asio_context, Queue<OwnedMsg>* incoming_messages, asio::ip::tcp::socket&& tcp_socket, std::uint32_t client_id);

            virtual ~ClientConnection() noexcept = default;

            ClientConnection(const ClientConnection&) = delete;
            ClientConnection& operator=(const ClientConnection&) = delete;
            ClientConnection(ClientConnection&&) = delete;
            ClientConnection& operator=(ClientConnection&&) = delete;

            virtual void try_connect() override;
            virtual std::uint32_t get_id() const override;
        private:
            virtual void add_to_incoming_messages() override;

            std::uint32_t client_id = 0;  // 0 is invalid
        };

        // Owner of this is the client
        class ServerConnection final : public Connection {
        public:
            ServerConnection(asio::io_context* asio_context, Queue<OwnedMsg>* incoming_messages, asio::ip::tcp::socket&& tcp_socket, const asio::ip::tcp::resolver::results_type& endpoints)
                : Connection(asio_context, incoming_messages, std::move(tcp_socket)), endpoints(endpoints) {}

            virtual ~ServerConnection() noexcept = default;

            ServerConnection(const ServerConnection&) = delete;
            ServerConnection& operator=(const ServerConnection&) = delete;
            ServerConnection(ServerConnection&&) = delete;
            ServerConnection& operator=(ServerConnection&&) = delete;

            virtual void try_connect() override;
        private:
            virtual void add_to_incoming_messages() override;

            void task_connect_to_server();

            asio::ip::tcp::resolver::results_type endpoints;
        };
    }
}
