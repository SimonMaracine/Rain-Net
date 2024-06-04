#pragma once

#include <thread>
#include <memory>
#include <string_view>
#include <cstdint>
#include <optional>

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
#include "rain_net/internal/connection.hpp"
#include "rain_net/internal/errorable.hpp"

namespace rain_net {
    // Base class for the client application
    class Client : public internal::Errorable {
    public:
        Client() = default;
        virtual ~Client();

        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;
        Client(Client&&) = delete;
        Client& operator=(Client&&) = delete;

        // Start the client's internal event loop and connect to the server
        // You may call this only once in the beginning or after calling disconnect()
        void connect(std::string_view host, std::uint16_t port);

        // Disconnect from the server and stop the internal event loop
        // You may call this only once after connecting
        // After a call to disconnect(), you may reconnect by calling connect() again
        // It is automatically called in the destructor
        void disconnect();

        // After a call to connect(), check if the connection has been established
        // You may call this in a loop
        bool connection_established() const;
    protected:
        // Poll the next message from the server; you usually do it in a loop until std::nullopt
        std::optional<Message> next_incoming_message();

        // Send a message to the server
        void send_message(const Message& message);

        // Don't touch these unless you really know what you're doing
        internal::SyncQueue<Message> incoming_messages;
        std::unique_ptr<ServerConnection> connection;
    private:
        std::thread context_thread;
        asio::io_context asio_context;
    };
}
