#pragma once

#include <utility>
#include <cstddef>

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
            Connection(asio::io_context& asio_context, asio::ip::tcp::socket&& tcp_socket)
                : m_asio_context(asio_context), m_tcp_socket(std::move(tcp_socket)) {}

            ~Connection() = default;

            Connection(const Connection&) = delete;
            Connection& operator=(const Connection&) = delete;
            Connection(Connection&&) = delete;
            Connection& operator=(Connection&&) = delete;

            void close();
            bool is_open() const;

            asio::io_context& m_asio_context;
            asio::ip::tcp::socket m_tcp_socket;

            internal::SyncQueue<internal::BasicMessage> m_outgoing_messages;
            internal::BasicMessage m_current_incoming_message;
        };

        template<typename T>
        std::size_t buffers_size(const T& buffers) noexcept {
            std::size_t size {0};

            for (const auto& buffer : buffers) {
                size += buffer.size();
            }

            return size;
        }
    }
}
