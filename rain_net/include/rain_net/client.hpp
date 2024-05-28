#pragma once

#include <thread>
#include <memory>
#include <string_view>
#include <cstdint>
#include <optional>
#include <functional>

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

        // Send a message to the server; return false, if nothing could be sent, true otherwise
        bool send_message(const Message& message);

        // Poll the next message from the server; you usually do it in a loop until std::nullopt
        std::optional<Message> next_incoming_message();

        // These are accessible to the public for inspection
        // Don't modify anything, if you don't know what you're doing
        internal::SyncQueue<internal::OwnedMsg<ServerConnection>> incoming_messages;
        std::unique_ptr<ServerConnection> connection;
    private:
        asio::io_context asio_context;
        std::thread context_thread;
    };
}
