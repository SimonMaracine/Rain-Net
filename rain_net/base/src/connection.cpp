#include "rain_net/internal/connection.hpp"

#ifdef __GNUG__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wconversion"
#endif

#include <asio/post.hpp>

#ifdef __GNUG__
    #pragma GCC diagnostic pop
#endif

namespace rain_net {
    namespace internal {
        void Connection::close() {
            asio::post(m_asio_context, [this]() {
                if (!m_tcp_socket.is_open()) {
                    return;
                }

                m_tcp_socket.close();
            });
        }

        bool Connection::is_open() const {
            return m_tcp_socket.is_open();
        }
    }
}
