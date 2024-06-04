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
    ThisServer()
        : rain_net::Server(log) {}

    bool on_client_connected(std::shared_ptr<rain_net::ClientConnection>) override {
        return true;
    }

    void on_client_disconnected(std::shared_ptr<rain_net::ClientConnection>) override {

    }

    void process_messages() {
        while (true) {
            const auto result {next_incoming_message()};

            if (!result) {
                break;
            }

            const auto& [connection, message] {*result};

            switch (message.id()) {
                case MsgType::PingServer:
                    std::cout << "Ping request from " << connection->get_id() << '\n';

                    // Just send the same message back
                    send_message(connection, message);

                    break;
            }
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

    ThisServer server;
    server.start(6001);

    if (server.fail()) {
        return 1;
    }

    while (running) {
        server.accept_connections();
        server.process_messages();

        if (server.fail()) {
            break;
        }
    }

    server.stop();
}
