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

// Include this also for the user
#include "rain_net/connection.hpp"

namespace rain_net {
    // Base class for the server program
    class Server {
    public:
        // Basically infinity
        static constexpr std::uint32_t MAX_MSG = std::numeric_limits<std::uint32_t>::max();

        // The port number is specified at creation time
        Server(std::uint16_t port)
            : listen_port(port), acceptor(asio_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {}

        virtual ~Server();

        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        Server(Server&&) = delete;
        Server& operator=(Server&&) = delete;

        // Start, stop the server; you should call stop()
        void start();
        void stop();

        // Call this in a loop to continuously receive messages
        // You can specify a maximum amount of processed messages before returning
        // Set wait to true, to put the CPU to sleep when there is no work to do
        void update(const std::uint32_t max_messages = MAX_MSG, bool wait = false);
    protected:
        // Return false to reject the client, true otherwise
        virtual bool on_client_connected(std::shared_ptr<Connection> client_connection) = 0;
        virtual void on_client_disconnected(std::shared_ptr<Connection> client_connection) = 0;
        virtual void on_message_received(std::shared_ptr<Connection> client_connection, Message& message) = 0;

        // Send message to a specific client, or to everyone except a specific client
        void send_message(std::shared_ptr<Connection> client_connection, const Message& message);
        void send_message_all(const Message& message, std::shared_ptr<Connection> exception = nullptr);

        // Routine to check all connections to see if they are valid
        void check_connections();

        // Data accessible to the derived class; don't touch these unless you really know what you're doing
        internal::WaitingQueue<internal::OwnedMsg> incoming_messages;
        std::deque<std::shared_ptr<Connection>> active_connections;
        std::uint16_t listen_port = 0;
    private:
        void task_wait_for_connection();

        asio::io_context asio_context;
        std::thread context_thread;

        asio::ip::tcp::acceptor acceptor;

        std::uint32_t client_id_counter = 0;  // ID 0 is invalid

        bool stoppable = false;
    };
}
