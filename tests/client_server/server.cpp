#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <csignal>

#include <rain_net/server.hpp>

enum MsgType : std::uint16_t {
    PingServer
};

static void log(const std::string& message) {
    std::cerr << message << '\n';
}

struct ThisServer : public rain_net::Server {
    explicit ThisServer(std::uint16_t port)
        : rain_net::Server(port, log) {}

    bool on_client_connected(std::shared_ptr<rain_net::ClientConnection>) override {
        return true;
    }

    void on_client_disconnected(std::shared_ptr<rain_net::ClientConnection>) override {

    }

    void on_message_received(std::shared_ptr<rain_net::ClientConnection> client_connection, const rain_net::Message& message) override {
        switch (message.id()) {
            case MsgType::PingServer:
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

    while (running) {
        server.update();

        if (server.fail()) {
            break;
        }
    }

    server.stop();
}
