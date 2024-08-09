#pragma once

#include <thread>
#include <memory>
#include <string_view>
#include <cstdint>
#include <exception>

#ifdef __GNUG__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wconversion"
#endif

#include <asio/io_context.hpp>

#ifdef __GNUG__
    #pragma GCC diagnostic pop
#endif

#include "rain_net/internal/queue.hpp"
#include "rain_net/internal/message.hpp"
#include "rain_net/internal/server_connection.hpp"

// Forward
#include "rain_net/internal/error.hpp"

namespace rain_net {
    // Base class for the client application
    class Client {
    public:
        Client() = default;
        ~Client();

        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;
        Client(Client&&) = delete;
        Client& operator=(Client&&) = delete;

        // Start the client's internal event loop and connect to the server
        // You may call this only once in the beginning or after calling disconnect()
        // Throws errors
        void connect(std::string_view host, std::uint16_t port);

        // Disconnect from the server and stop the internal event loop
        // You may call this at any time
        // After a call to disconnect(), you may reconnect by calling connect() again
        // Clears the error flag
        // It is automatically called in the destructor
        void disconnect();

        // After a call to connect(), check if the connection has been established
        // You may call this in a loop
        // Throws errors
        bool connection_established() const;

        // Poll the next incoming message from the queue
        // You may call it in a loop to process as many messages as you want
        // Throws errors
        Message next_message();

        // Check if there are available incoming messages
        bool available() const;

        // Send a message to the server
        // Does nothing, if the connection is not established
        void send_message(const Message& message);
    private:
        internal::SyncQueue<Message> incoming_messages;
        std::unique_ptr<ServerConnection> connection;

        std::thread context_thread;
        asio::io_context asio_context;

        std::exception_ptr error;
    };
}
