#pragma once

#include <cstdint>
#include <memory>
#include <thread>
#include <forward_list>
#include <limits>
#include <optional>

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

namespace rain_net {
    class ClientsPool final {
    public:
        ClientsPool() = default;
        ClientsPool(std::uint32_t size);
        ~ClientsPool();

        ClientsPool(const ClientsPool&) = delete;
        ClientsPool& operator=(const ClientsPool&) = delete;
        ClientsPool(ClientsPool&& other) noexcept;
        ClientsPool& operator=(ClientsPool&& other) noexcept;

        std::optional<std::uint32_t> allocate_id();
        void deallocate_id(std::uint32_t id);
    private:
        void create_pool(std::uint32_t size);
        void destroy_pool();
        std::optional<std::uint32_t> search_and_allocate_id(std::uint32_t begin, std::uint32_t end);

        bool* pool {nullptr};  // False means it's not allocated
        std::uint32_t id_pointer {};
        std::uint32_t size {};
    };

    // Base class for the server program
    class Server {
    public:
        // Basically infinity
        static constexpr std::uint32_t MAX_MSG {std::numeric_limits<std::uint32_t>::max()};

        // The port number is specified at creation time
        explicit Server(std::uint16_t port)
            : listen_port(port), acceptor(asio_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {}

        virtual ~Server();

        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        Server(Server&&) = delete;
        Server& operator=(Server&&) = delete;

        // Start, stop the server; you should call stop()
        // Here you can specify the maximum amount of connected clients allowed
        void start(std::uint32_t max_clients = std::numeric_limits<std::uint16_t>::max());
        void stop();

        // Call this in a loop to continuously receive messages
        // You can specify a maximum amount of processed messages before returning
        // Set wait to true, to put the CPU to sleep when there is no work to do
        void update(const std::uint32_t max_messages = MAX_MSG, bool wait = true);
    protected:
        // Return false to reject the client, true otherwise
        virtual bool on_client_connected(std::shared_ptr<ClientConnection> client_connection) = 0;
        virtual void on_client_disconnected(std::shared_ptr<ClientConnection> client_connection) = 0;
        virtual void on_message_received(std::shared_ptr<ClientConnection> client_connection, const Message& message) = 0;

        // Send message to a specific client, or to everyone except a specific client
        void send_message(std::shared_ptr<ClientConnection> client_connection, const Message& message);
        void send_message_all(const Message& message, std::shared_ptr<ClientConnection> exception = nullptr);

        // Routine to check all connections to see if they are valid
        void check_connections();

        // Data accessible to the derived class; don't touch these unless you really know what you're doing
        internal::WaitingSyncQueue<internal::OwnedMsg<ClientConnection>> incoming_messages;
        std::forward_list<std::shared_ptr<ClientConnection>> active_connections;
        std::uint16_t listen_port {};
    private:
        void task_wait_for_connection();
        void create_new_connection(asio::ip::tcp::socket&& socket, std::uint32_t id);

        asio::io_context asio_context;
        std::thread context_thread;

        asio::ip::tcp::acceptor acceptor;

        ClientsPool clients;

        bool stoppable {false};
    };
}
