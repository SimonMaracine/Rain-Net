#pragma once

#include <cstdint>
#include <memory>
#include <thread>
#include <forward_list>
#include <limits>
#include <optional>
#include <string>
#include <utility>
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
#include "rain_net/internal/pool_clients.hpp"

// Forward
#include "rain_net/internal/error.hpp"

namespace rain_net {
    // Base class for the server program
    class Server {
    public:
        // Default capacity of clients
        static constexpr std::uint32_t MAX_CLIENTS {std::numeric_limits<std::uint16_t>::max()};

        Server()
            : acceptor(asio_context) {}

        virtual ~Server();

        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        Server(Server&&) = delete;
        Server& operator=(Server&&) = delete;

        // Start the internal event loop and start accepting connection requests
        // You may call this only once in the beginning or after calling stop()
        // Specify the port number on which to listen and the maximum amount of clients allowed
        void start(std::uint16_t port, std::uint32_t max_clients = MAX_CLIENTS);

        // Disconnect from all the clients and stop the internal event loop
        // You may call this at any time
        // After a call to stop(), you may restart by calling start() again
        // Clears the error flag
        // It is automatically called in the destructor
        void stop();

        // Check if there are available incoming messages without polling them
        bool available() const;

        // Accept new connections; invokes on_client_connected(); you must call this regularly
        void accept_connections();

        // Check all connections to see if they are valid; invokes on_client_disconnected() when needed
        // You may not really need it
        void check_connections();

        // Poll the next message from the clients; you usually do it in a loop until std::nullopt
        std::optional<std::pair<Message, std::shared_ptr<ClientConnection>>> next_incoming_message();

        // Send a message to a specific client; invokes on_client_disconnected() when needed
        void send_message(std::shared_ptr<ClientConnection> connection, const Message& message);

        // Send a message to all clients; invokes on_client_disconnected() when needed
        void send_message_broadcast(const Message& message);

        // Send a message to all clients except a specific client; invokes on_client_disconnected() when needed
        void send_message_broadcast(const Message& message, std::shared_ptr<ClientConnection> exception);
    protected:
        // Called for certain events
        virtual void on_log(const std::string& message);

        // Called when a new client tries to connect; return false to reject the client
        virtual bool on_client_connected(std::shared_ptr<ClientConnection> connection) = 0;

        // Called when a client disconnection is detected
        virtual void on_client_disconnected(std::shared_ptr<ClientConnection> connection) = 0;
    private:
        void task_accept_connection();

        internal::SyncQueue<std::pair<Message, std::shared_ptr<ClientConnection>>> incoming_messages;
        std::forward_list<std::shared_ptr<ClientConnection>> connections;
        internal::SyncQueue<std::shared_ptr<ClientConnection>> new_connections;

        std::thread context_thread;
        asio::io_context asio_context;
        asio::ip::tcp::acceptor acceptor;

        internal::PoolClients clients;
        std::exception_ptr error;
        bool running {false};
    };
}
