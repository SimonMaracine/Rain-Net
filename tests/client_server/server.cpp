#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <csignal>

#include <rain_net/server.hpp>

enum class MsgType : std::uint16_t {
    PingServer
};

struct ThisServer : public rain_net::Server {
    explicit ThisServer(std::uint16_t port)
        : rain_net::Server(port) {}

    bool on_client_connected(std::shared_ptr<rain_net::Connection> client_connection) override {
        std::cout << "Yaay... " << client_connection->get_id() << '\n';

        return true;
    }

    void on_client_disconnected(std::shared_ptr<rain_net::Connection> client_connection) override {
        std::cout << "Removed client " << client_connection->get_id() << '\n';
    }

    void on_message_received(std::shared_ptr<rain_net::Connection> client_connection, rain_net::Message& message) override {
        switch (message.id()) {
            case rain_net::id(MsgType::PingServer):
                std::cout << "Ping request from " << client_connection->get_id() << '\n';

                // Just send the same message back
                send_message(client_connection, message);

                break;
        }
    }
};

static volatile bool running {true};

int main() {
    const auto handler {
        [](int) { running = false; }
    };

    if (std::signal(SIGINT, handler) == SIG_ERR) {
        std::abort();
    }

    ThisServer server {6001};
    server.start();

    std::cout << "Started\n";

    while (running) {
        server.update(ThisServer::MAX_MSG, true);
        std::cout << "Returned from update\n";
    }

    server.stop();

    std::cout << "Stopped\n";
}
