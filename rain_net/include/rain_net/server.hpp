#pragma once

#include <cstdint>
#include <memory>
#include <thread>
#include <forward_list>
#include <limits>
#include <optional>
#include <functional>
#include <string>
#include <utility>

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
#include "rain_net/internal/connection.hpp"
#include "rain_net/internal/errorable.hpp"

namespace rain_net {
    namespace internal {
        class PoolClients final {
        public:
            PoolClients() = default;
            explicit PoolClients(std::uint32_t size);
            ~PoolClients() = default;

            PoolClients(const PoolClients&) = delete;
            PoolClients& operator=(const PoolClients&) = delete;
            PoolClients(PoolClients&& other) noexcept = default;
            PoolClients& operator=(PoolClients&& other) noexcept = default;

            std::optional<std::uint32_t> allocate_id();
            void deallocate_id(std::uint32_t id);
        private:
            void create_pool(std::uint32_t size);
            std::optional<std::uint32_t> search_and_allocate_id(std::uint32_t begin, std::uint32_t end);

            std::unique_ptr<bool[]> pool;  // False means it's not allocated
            std::uint32_t id_pointer {};
            std::uint32_t size {};
        };
    }

    // Base class for the server program
    class Server : public internal::Errorable {
    public:
        // Empty log procedure
        static constexpr auto NO_LOG {[](std::string&&) {}};

        // Default capacity of clients
        static constexpr std::uint32_t MAX_CLIENTS {std::numeric_limits<std::uint16_t>::max()};

        // Specify port number on which to listen and log procedure
        Server(std::uint16_t port, std::function<void(std::string&&)> log_fn = NO_LOG)
            : acceptor(asio_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)), log_fn(std::move(log_fn)) {}

        virtual ~Server();

        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        Server(Server&&) = delete;
        Server& operator=(Server&&) = delete;

        // Start the internal event loop and start accepting connection requests
        // You may call this only once in the beginning or after calling stop()
        void start(std::uint32_t max_clients = MAX_CLIENTS);

        // Disconnect from all the clients and stop the internal event loop
        // You may call this only once after starting
        // After a call to stop(), you may restart by calling start() again
        // It is automatically called in the destructor
        void stop();

        // Poll the next message from the clients; you usually do it in a loop until std::nullopt
        std::optional<std::pair<std::shared_ptr<ClientConnection>, Message>> next_incoming_message();

        // Check if there are available incoming messages without polling them
        bool available() const;  // TODO
    protected:
        // Called when a new client tries to connect; return false to reject the client, true otherwise
        // Called from the server's thread; be aware of race conditions  // FIXME
        virtual bool on_client_connected(std::shared_ptr<ClientConnection> connection) = 0;

        // Called when a client disconnection is detected
        // Called from the main thread
        virtual void on_client_disconnected(std::shared_ptr<ClientConnection> connection) = 0;

        // Send a message to a specific client
        void send_message(std::shared_ptr<ClientConnection> connection, const Message& message);

        // Senf a message to all clients
        void send_message_broadcast(const Message& message);

        // Send a message to all clients except a specific client
        void send_message_broadcast(const Message& message, std::shared_ptr<ClientConnection> exception);

        // Check all connections to see if they are valid; invokes on_client_disconnected() when needed
        void check_connections();

        // Don't touch these unless you really know what you're doing
        internal::SyncQueue<internal::OwnedMsg> incoming_messages;
        std::forward_list<std::shared_ptr<ClientConnection>> active_connections;
    private:
        void task_accept_connection();
        void create_new_connection(asio::ip::tcp::socket&& socket, std::uint32_t id);

        std::thread context_thread;
        asio::io_context asio_context;
        asio::ip::tcp::acceptor acceptor;

        internal::PoolClients clients;
        std::function<void(std::string&&)> log_fn;
        bool running {false};
    };
}
