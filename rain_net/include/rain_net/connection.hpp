#pragma once

#include <asio/ts/internet.hpp>

#include "rain_net/message.hpp"
#include "rain_net/queue.hpp"

namespace rain_net {
    template<typename E>
    class Connection final {
    public:
        bool connect() { return false; }  // TODO to server?
        bool disconnect() { return false; }
        bool is_connected() const { return false; }

        bool send(const Message<E>& message) { return false; }
    private:
        asio::ip::tcp::socket socket;
        asio::io_context* asio_context = nullptr;

        Queue<Message<E>> outgoing_messages;

        Queue<OwnedMessage<E>>* incoming_messages = nullptr;
    };
}
