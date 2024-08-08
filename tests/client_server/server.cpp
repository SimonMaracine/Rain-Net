#include <iostream>
#include <cstdint>
#include <csignal>

#include <rain_net/server.hpp>

enum MsgType : std::uint16_t {
    PingServer
};

struct ThisServer : public rain_net::Server {
    void on_log(const std::string& message) override {
        std::cerr << message << '\n';
    }

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

            const auto& [message, connection] {*result};

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
        return 1;
    }

    ThisServer server;

    try {
        server.start(6001);

        while (running) {
            server.accept_connections();
            server.process_messages();
        }
    } catch (const rain_net::ConnectionError& e) {
        return 1;
    }

    server.stop();

    return 0;
}
