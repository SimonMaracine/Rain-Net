#pragma once

#include <thread>
#include <memory>
#include <string_view>
#include <cstdint>
#include <optional>
#include <functional>

#define ASIO_NO_DEPRECATED
#include <asio/io_context.hpp>

#include "rain_net/queue.hpp"
#include "rain_net/message.hpp"

// Include this also for the user
#include "rain_net/connection.hpp"

namespace rain_net {
    // Base class for the client application
    class Client {
    public:
        using OnConnected = std::function<void()>;

        Client() = default;
        virtual ~Client();

        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;
        Client(Client&&) = delete;
        Client& operator=(Client&&) = delete;

        // Connect, disconnect and check connection to server
        // on_connected is called when the connection is established; be aware of race conditions
        bool connect(std::string_view host, std::uint16_t port, const OnConnected& on_connected = []() {});
        void disconnect();
        bool is_connected() const;

        // Send a message to the server
        void send_message(const Message& message);

        // Poll the next message from the server; you usually do it in a loop until std::nullopt
        std::optional<Message> next_incoming_message();

        // These are accessible to the public for inspection
        // Don't modify anything, if you don't know what you're doing
        internal::Queue<internal::OwnedMsg> incoming_messages;
        std::unique_ptr<Connection> connection;
    private:
        asio::io_context asio_context;
        std::thread context_thread;
    };
}
