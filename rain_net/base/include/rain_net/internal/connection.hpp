#pragma once

#include <utility>
#include <cstddef>
#include <memory>

#ifdef __GNUG__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wconversion"
#endif

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

#ifdef __GNUG__
    #pragma GCC diagnostic pop
#endif

#include "rain_net/internal/message.hpp"
#include "rain_net/internal/queue.hpp"

namespace rain_net {
    namespace internal {
        class Connection {
        protected:
            void close();
            bool is_open() const;

            Connection(asio::io_context& asio_context, asio::ip::tcp::socket&& tcp_socket)
                : asio_context(asio_context), tcp_socket(std::move(tcp_socket)) {}

            ~Connection() = default;

            Connection(const Connection&) = delete;
            Connection& operator=(const Connection&) = delete;
            Connection(Connection&&) = delete;
            Connection& operator=(Connection&&) = delete;

            asio::io_context& asio_context;
            asio::ip::tcp::socket tcp_socket;

            internal::SyncQueue<internal::BasicMessage> outgoing_messages;
            internal::BasicMessage current_incoming_message;
        };

        template<typename T>
        std::size_t buffers_size(const T& buffers) {
            std::size_t size {0};

            for (const auto& buffer : buffers) {
                size += buffer.size();
            }

            return size;
        }
    }
}
