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
        // Basically infinity
        static constexpr std::uint32_t MAX_MSG {std::numeric_limits<std::uint32_t>::max()};

        static constexpr auto NO_LOG {[](std::string&&) {}};

        // The port number is specified at creation time
        Server(std::uint16_t port, std::function<void(std::string&&)> log_fn = NO_LOG)
            : acceptor(asio_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)), log_fn(std::move(log_fn)) {}

        virtual ~Server() = default;

        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        Server(Server&&) = delete;
        Server& operator=(Server&&) = delete;

        // Start, stop the server; you should call stop() only after calling start()
        // Calling start() then restarts the server
        // Here you can specify the maximum amount of connected clients allowed
        void start(std::uint32_t max_clients = std::numeric_limits<std::uint16_t>::max());
        void stop();

        // Call this in a loop to continuously receive messages
        // You can specify a maximum amount of processed messages before returning
        // Set wait to false, to not put the CPU to sleep when there is no work to do
        void update(std::uint32_t max_messages = MAX_MSG);

        bool available() const;  // TODO
    protected:
        // Called when a new client tries to connect; return false to reject the client, true otherwise
        virtual bool on_client_connected(std::shared_ptr<ClientConnection> client_connection) = 0;

        // Called when a client disconnection is detected
        virtual void on_client_disconnected(std::shared_ptr<ClientConnection> client_connection) = 0;

        // Called for every message received from every client
        virtual void on_message_received(std::shared_ptr<ClientConnection> client_connection, const Message& message) = 0;

        // Send message to a specific client; return false, if nothing could be sent, true otherwise
        bool send_message(std::shared_ptr<ClientConnection> client_connection, const Message& message);

        void send_message_broadcast(const Message& message);

        // Send a message to everyone except a specific client
        void send_message_broadcast(const Message& message, std::shared_ptr<ClientConnection> exception);

        // Routine to check all connections to see if they are valid
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
