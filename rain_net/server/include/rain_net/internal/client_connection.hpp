#pragma once

#include <utility>
#include <memory>
#include <cstdint>
#include <functional>
#include <string>

#include "rain_net/internal/connection.hpp"

namespace rain_net {
    class Server;

    // Owner of this is the server
    class ClientConnection final : public internal::Connection, public std::enable_shared_from_this<ClientConnection> {
    public:
        ClientConnection(
            asio::io_context& asio_context,
            asio::ip::tcp::socket&& tcp_socket,
            internal::SyncQueue<std::pair<Message, std::shared_ptr<ClientConnection>>>& incoming_messages,
            std::uint32_t client_id,
            const std::function<void(const std::string&)>& log
        )
            : internal::Connection(asio_context, std::move(tcp_socket)), m_incoming_messages(incoming_messages),
            m_log(log), m_client_id(client_id) {}

        // Send a message asynchronously
        void send(const Message& message);

        // Get the unique ID of this client
        std::uint32_t get_id() const noexcept;
    private:
        void start_communication();
        void add_to_incoming_messages();

        void task_write_message();
        void task_read_header();
        void task_read_payload();
        void task_send_message(const Message& message);

        internal::SyncQueue<std::pair<Message, std::shared_ptr<ClientConnection>>>& m_incoming_messages;
        const std::function<void(const std::string&)>& m_log;
        std::uint32_t m_client_id {};  // Given by the server
        bool m_used {false};  // Set to true after using the connection and calling on_client_disconnected()

        friend class Server;
    };
}
