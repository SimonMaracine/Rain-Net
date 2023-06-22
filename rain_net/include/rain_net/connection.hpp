#pragma once

#include <utility>
#include <memory>

#include <asio.hpp>
#include <asio/ts/internet.hpp>
#include <asio/ts/buffer.hpp>

#include "message.hpp"
#include "queue.hpp"

namespace rain_net {
    template<typename E>
    class Connection {
    public:
        Connection(asio::io_context* asio_context, Queue<OwnedMessage<E>>* incoming_messages, asio::ip::tcp::socket&& tcp_socket)
            : asio_context(asio_context), incoming_messages(incoming_messages), tcp_socket(std::move(tcp_socket)) {}

        virtual ~Connection() = default;

        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;
        Connection(Connection&&) = delete;
        Connection& operator=(Connection&&) = delete;

        virtual void connect() = 0;

        virtual uint32_t get_id() const {
            return 0;
        }

        void disconnect() {
            if (is_connected()) {
                task_close_socket();
            }
        }

        bool is_connected() const {
            return tcp_socket.is_open();
        }

        void send(const Message<E>& message) {
            task_send_message(message);
        }
    protected:
        virtual void add_to_incoming_messages() = 0;

        void task_read_header() {
            static_assert(std::is_trivially_copyable_v<MsgHeader<E>>);  // FIXME good?

            asio::async_read(tcp_socket, asio::buffer(&current_incoming_message.header, sizeof(MsgHeader<E>)),
                [this](asio::error_code ec, size_t size) {
                    if (ec) {
                        std::cout << "Could not read header (" << get_id() <<  ")\n";

                        // Close the connection on this side; the remote will pick this up
                        tcp_socket.close();
                    } else {
                        // TODO does it read it all?
                        assert(size == sizeof(MsgHeader<E>));

                        // Check if there is a payload to read
                        if (current_incoming_message.header.payload_size > 0) {
                            // Allocate space so that we write to it later
                            current_incoming_message.payload.resize(current_incoming_message.header.payload_size);

                            task_read_payload();
                        } else {
                            add_to_incoming_messages();
                            task_read_header();
                        }
                    }
                }
            );
        }

        void task_read_payload() {
            asio::async_read(tcp_socket, asio::buffer(current_incoming_message.payload.data(), current_incoming_message.header.payload_size),
                [this](asio::error_code ec, size_t size) {
                    if (ec) {
                        std::cout << "Could not read payload (" << get_id() << ")\n";

                        // Close the connection on this side; the remote will pick this up
                        tcp_socket.close();
                    } else {
                        add_to_incoming_messages();
                        task_read_header();
                    }
                }
            );
        }

        void task_write_header() {
            static_assert(std::is_trivially_copyable_v<MsgHeader<E>>);  // FIXME good?

            asio::async_write(tcp_socket, asio::buffer(&outgoing_messages.front().header, sizeof(MsgHeader<E>)),
                [this](asio::error_code ec, size_t size) {
                    if (ec) {
                        std::cout << "Could not write header (" << get_id() << ")\n";

                        // Close the connection on this side; the remote will pick this up
                        tcp_socket.close();
                    } else {
                        // Check if there is a payload to read
                        if (outgoing_messages.front().header.payload_size > 0) {
                            task_write_payload();
                        } else {
                            outgoing_messages.pop_front();

                            if (!outgoing_messages.empty()) {  // Thus writing tasks can stop
                                task_write_header();
                            }
                        }
                    }
                }
            );
        }

        void task_write_payload() {
            asio::async_write(tcp_socket, asio::buffer(outgoing_messages.front().payload.data(), outgoing_messages.front().header.payload_size),
                [this](asio::error_code ec, size_t size) {
                    if (ec) {
                        std::cout << "Could not write payload (" << get_id() << ")\n";

                        // Close the connection on this side; the remote will pick this up
                        tcp_socket.close();
                    } else {
                        outgoing_messages.pop_front();

                        if (!outgoing_messages.empty()) {  // Thus writing tasks can stop
                            task_write_header();
                        }
                    }
                }
            );
        }

        void task_send_message(const Message<E>& message) {
            // This really needs to be a task
            asio::post(asio_context,
                [this, &message]() {
                    const bool writing_tasks_stopped = outgoing_messages.empty();

                    outgoing_messages.push_bask(message);

                    // Restart the writing process, if it has stopped before
                    if (writing_tasks_stopped) {
                        task_write_header();
                    }
                }
            );
        }

        void task_close_socket() {
            asio::post(asio_context,
                [this]() {
                    tcp_socket.close();
                }
            );
        }

        asio::io_context* asio_context = nullptr;
        Queue<OwnedMessage<E>>* incoming_messages = nullptr;

        asio::ip::tcp::socket tcp_socket;
        Queue<Message<E>> outgoing_messages;

        Message<E> current_incoming_message;
    };

    // Owner of this is the server
    template<typename E>
    class ClientConnection final : public Connection<E> {
    public:
        ClientConnection(asio::io_context* asio_context, Queue<OwnedMessage<E>>* incoming_messages, asio::ip::tcp::socket&& tcp_socket, uint32_t client_id)
            : Connection<E>(asio_context, incoming_messages, std::move(tcp_socket)), client_id(client_id) {}

        virtual ~ClientConnection() = default;

        ClientConnection(const ClientConnection&) = delete;
        ClientConnection& operator=(const ClientConnection&) = delete;
        ClientConnection(ClientConnection&&) = delete;
        ClientConnection& operator=(ClientConnection&&) = delete;

        virtual void connect() override {  // Connect to client
            if (!this->tcp_socket.is_open()) {
                return;
            }

            this->task_read_header();
        }

        virtual uint32_t get_id() const override {
            return client_id;
        }
    private:
        virtual void add_to_incoming_messages() override {
            OwnedMessage<E> owned_message;
            owned_message.msg = this->current_incoming_message;
            owned_message.remote = nullptr;  // This distinction is important

            this->incoming_messages->push_back(std::move(owned_message));
        }

        uint32_t client_id = 0;
    };

    // Owner of this is the client
    template<typename E>
    class ServerConnection final : public Connection<E>, std::enable_shared_from_this<ServerConnection<E>> {
    public:
        ServerConnection(asio::io_context* asio_context, Queue<OwnedMessage<E>>* incoming_messages, asio::ip::tcp::socket&& tcp_socket, asio::ip::tcp::resolver::results_type&& endpoints)
            : Connection<E>(asio_context, incoming_messages, std::move(tcp_socket)), endpoints(std::move(endpoints)) {}

        virtual ~ServerConnection() = default;

        ServerConnection(const ServerConnection&) = delete;
        ServerConnection& operator=(const ServerConnection&) = delete;
        ServerConnection(ServerConnection&&) = delete;
        ServerConnection& operator=(ServerConnection&&) = delete;

        virtual void connect() override {  // Connect to server
            task_connect_to_server();;
        }
    private:
        virtual void add_to_incoming_messages() override {
            OwnedMessage<E> owned_message;
            owned_message.msg = this->current_incoming_message;
            owned_message.remote = this->shared_from_this();  // This distinction is important

            this->incoming_messages->push_back(std::move(owned_message));
        }

        void task_connect_to_server() {
            asio::async_connect(this->tcp_socket, endpoints,
                [this](asio::error_code ec, asio::ip::tcp::endpoint endpoint) {
                    if (ec) {
                        std::cout << "Could not connect to the server\n";

                        return;
                    }

                    this->task_read_header();
                }
            );
        }

        asio::ip::tcp::resolver::results_type endpoints;
    };
}
