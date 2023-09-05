#pragma once

#include <cstdint>
#include <memory>
#include <thread>
#include <deque>
#include <limits>

#define ASIO_NO_DEPRECATED
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

#include "rain_net/queue.hpp"
#include "rain_net/message.hpp"
#include "rain_net/connection.hpp"

namespace rain_net {
    class Server {
    public:
        static constexpr std::uint32_t MAX = std::numeric_limits<std::uint32_t>::max();

        Server(std::uint16_t port)
            : listen_port(port), acceptor(asio_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {}

        virtual ~Server();

        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        Server(Server&&) = delete;
        Server& operator=(Server&&) = delete;

        void start();
        void stop();
        void update(const std::uint32_t max_messages = MAX, bool wait = false);
    protected:
        // Return false to reject the client, true otherwise
        virtual bool on_client_connected(std::shared_ptr<Connection> client_connection) = 0;
        virtual void on_client_disconnected(std::shared_ptr<Connection> client_connection) = 0;
        virtual void on_message_received(std::shared_ptr<Connection> client_connection, Message& message) = 0;

        void send_message(std::shared_ptr<Connection> client_connection, const Message& message);
        void send_message_all(const Message& message, std::shared_ptr<Connection> except = nullptr);

        internal::WaitingQueue<internal::OwnedMsg> incoming_messages;
        std::deque<std::shared_ptr<Connection>> active_connections;  // TODO deque?
        std::uint16_t listen_port = 0;
    private:
        void task_wait_for_connection();

        asio::io_context asio_context;
        std::thread context_thread;

        asio::ip::tcp::acceptor acceptor;

        std::uint32_t client_id_counter = 0;  // 0 is invalid
    };
}
