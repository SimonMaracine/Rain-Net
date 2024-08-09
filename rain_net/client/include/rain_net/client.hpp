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
        virtual ~Client();

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

        // Update the client by processing messages; you must call this regularly
        // Invokes on_connected() and on_message_received() when needed
        // Throws errors
        void update();

        bool connection_established() const noexcept;

        // Send a message to the server
        void send_message(const Message& message);
    protected:
        virtual void on_message_received(const Message& message) = 0;
    private:
        void process_messages();

        internal::SyncQueue<Message> incoming_messages;
        std::unique_ptr<ServerConnection> connection;

        std::thread context_thread;
        asio::io_context asio_context;

        std::exception_ptr error;
    };
}
