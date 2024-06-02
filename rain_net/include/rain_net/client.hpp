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

        // Connect to the server; you should call disconnect() only after calling connect()
        // Calling connect() then reconnects to the server
        // on_connected is called when the connection is established; be aware of race conditions
        // connect() returns false when the host could not be resolved
        void connect(std::string_view host, std::uint16_t port);

        // Disconnect from the server
        void disconnect();

        bool is_connected() const;

        // Return true if the socket is open, false otherwise
        bool is_socket_open() const;

        // Send a message to the server; return false, if nothing could be sent, true otherwise
        void send_message(const Message& message);

        // Poll the next message from the server; you usually do it in a loop until std::nullopt
        std::optional<Message> next_incoming_message();

        // Don't touch these unless you really know what you're doing
        internal::SyncQueue<Message> incoming_messages;
        std::unique_ptr<ServerConnection> connection;
    private:
        std::thread context_thread;
        asio::io_context asio_context;
    };
}
