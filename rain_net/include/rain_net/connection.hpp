#pragma once

#include <utility>

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

        virtual bool connect() = 0;  // TODO to server?
        virtual bool disconnect() = 0;
        virtual bool is_connected() const = 0;

        virtual bool send(const Message<E>& message) = 0;
    protected:
        virtual void task_read_header() = 0;
        virtual void task_read_payload() = 0;
        virtual void task_write_header() = 0;
        virtual void task_write_payload() = 0;

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

        virtual bool connect() override {  // TODO to client?
            if (!this->tcp_socket.is_open()) {
                return false;
            }

            return true;
        }

        virtual bool disconnect() override {
            return false;
        }

        virtual bool is_connected() const override {
            return this->tcp_socket.is_open();
        }

        virtual bool send(const Message<E>& message) override {
            return false;
        }
    private:
        virtual void task_read_header() override {
            static_assert(std::is_trivially_copyable_v<MsgHeader<E>>);  // FIXME good?

            asio::async_read(this->tcp_socket, asio::buffer(&this->current_incoming_message.header, sizeof(MsgHeader<E>)),
                [this](asio::error_code ec, size_t size) {
                    if (ec) {
                        std::cout << "Could not read header (" << client_id << ")\n";

                        // Close the connection on this side; the remote will pick this up
                        this->tcp_socket.close();
                    } else {
                        // TODO does it read it?

                        if (this->current_incoming_message.header.payload_size > 0) {
                            // This means there is a payload to read
                            this->current_incoming_message.payload.resize(this->current_incoming_message.header.payload_size);

                            task_read_payload();
                        } else {
                            // TODO add to incoming_messages
                        }
                    }
                }
            );
        }

        virtual void task_read_payload() override {

        }

        virtual void task_write_header() override {

        }

        virtual void task_write_payload() override {

        }

        uint32_t client_id = 0;
    };

    // Owner of this is the client
    template<typename E>
    class ServerConnection final : public Connection<E> {
    public:
        ServerConnection(asio::io_context* asio_context, Queue<OwnedMessage<E>>* incoming_messages, asio::ip::tcp::socket&& tcp_socket)
            : Connection<E>(asio_context, incoming_messages, std::move(tcp_socket)) {}

        virtual ~ServerConnection() = default;

        ServerConnection(const ServerConnection&) = delete;
        ServerConnection& operator=(const ServerConnection&) = delete;
        ServerConnection(ServerConnection&&) = delete;
        ServerConnection& operator=(ServerConnection&&) = delete;

        virtual bool connect() override { return false; }  // TODO to server?
        virtual bool disconnect() override { return false; }
        virtual bool is_connected() const override { return false; }

        virtual bool send(const Message<E>& message) override { return false; }
    private:
        virtual void task_read_header() override {

        }

        virtual void task_read_payload() override {

        }

        virtual void task_write_header() override {

        }

        virtual void task_write_payload() override {

        }
    };
}
