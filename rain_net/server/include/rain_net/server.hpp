#pragma once

#include <cstdint>
#include <memory>
#include <thread>
#include <forward_list>
#include <limits>
#include <string>
#include <utility>
#include <functional>
#include <exception>

#ifdef __GNUG__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wconversion"
#endif

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

#ifdef __GNUG__
    #pragma GCC diagnostic pop
#endif

#include "rain_net/internal/queue.hpp"
#include "rain_net/internal/message.hpp"
#include "rain_net/internal/client_connection.hpp"
#include "rain_net/internal/pool.hpp"

// Forward
#include "rain_net/internal/error.hpp"

namespace rain_net {
    // Base class for the server program
    class Server final {
    public:
        // Default capacity of clients
        static constexpr std::uint32_t MAX_CLIENTS {std::numeric_limits<std::uint16_t>::max()};

        static constexpr auto ON_LOG {[](const std::string&) {}};

        Server(
            std::function<bool(Server&, std::shared_ptr<ClientConnection>)> on_client_connected,
            std::function<void(Server&, std::shared_ptr<ClientConnection>)> on_client_disconnected,
            std::function<void(const std::string&)> on_log = ON_LOG
        )
            : m_acceptor(m_asio_context), m_on_client_connected(std::move(on_client_connected)),
            m_on_client_disconnected(std::move(on_client_disconnected)), m_on_log(std::move(on_log)) {}

        ~Server();

        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        Server(Server&&) = delete;
        Server& operator=(Server&&) = delete;

        // Start the internal event loop and start accepting connection requests
        // You may call this only once in the beginning or after calling stop()
        // Specify the port number on which to listen and the maximum amount of clients allowed
        // Throws connection errors
        void start(std::uint16_t port, std::uint32_t max_clients = MAX_CLIENTS);

        // Disconnect from all the clients and stop the internal event loop
        // You may call this at any time
        // After a call to stop(), you may restart by calling start() again
        // It is automatically called in the destructor
        void stop();

        // Accepting new connections; you must call this regularly
        // Invokes on_client_connected() when needed
        // Throws connection errors
        void accept_connections();

        // Poll the next incoming message from the queue
        // You may call it in a loop to process as many messages as you want
        // Throws connection errors
        std::pair<Message, std::shared_ptr<ClientConnection>> next_message();

        // Check if there are available incoming messages
        bool available_messages() const;

        // Check all connections to see if they are valid; invokes on_client_disconnected() when needed
        // You may not really need it
        // Throws connection errors
        void check_connections();

        // Send a message to a specific client; invokes on_client_disconnected() when needed
        void send_message(std::shared_ptr<ClientConnection> connection, const Message& message);

        // Send a message to all clients; invokes on_client_disconnected() when needed
        void send_message_broadcast(const Message& message);

        // Send a message to all clients except a specific client; invokes on_client_disconnected() when needed
        void send_message_broadcast(const Message& message, std::shared_ptr<ClientConnection> exception);
    private:
        using ConnectionsIter = std::forward_list<std::shared_ptr<ClientConnection>>::iterator;

        void throw_if_error();
        void task_accept_connection();
        void maybe_client_disconnected(std::shared_ptr<ClientConnection> connection);
        bool maybe_client_disconnected(std::shared_ptr<ClientConnection> connection, ConnectionsIter& iter, ConnectionsIter before_iter);

        std::forward_list<std::shared_ptr<ClientConnection>> m_connections;
        internal::SyncQueue<std::shared_ptr<ClientConnection>> m_new_connections;
        internal::SyncQueue<std::pair<Message, std::shared_ptr<ClientConnection>>> m_incoming_messages;

        std::thread m_context_thread;
        asio::io_context m_asio_context;
        asio::ip::tcp::acceptor m_acceptor;

        std::function<bool(Server&, std::shared_ptr<ClientConnection>)> m_on_client_connected;
        std::function<void(Server&, std::shared_ptr<ClientConnection>)> m_on_client_disconnected;
        std::function<void(const std::string&)> m_on_log;

        internal::Pool m_pool;
        std::exception_ptr m_error;
        bool m_running {false};
    };
}
