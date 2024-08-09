#include <iostream>
#include <csignal>

#include <rain_net/server.hpp>

enum MessgeType {
    PingServer
};

static bool on_client_connected(rain_net::Server&, std::shared_ptr<rain_net::ClientConnection>) {
    return true;
}

static void on_client_disconnected(rain_net::Server&, std::shared_ptr<rain_net::ClientConnection>) {

}

static void on_log(const std::string& message) {
    std::cerr << message << '\n';
}

static void handle_message(rain_net::Server& server, const rain_net::Message& message, std::shared_ptr<rain_net::ClientConnection> connection) {
    switch (message.id()) {
        case MessgeType::PingServer:
            std::cout << "Ping request from " << connection->get_id() << '\n';

            // Just send the same message back
            server.send_message(connection, message);

            break;
    }
}

static volatile bool running {true};

int main() {
    const auto handler {
        [](int) { running = false; }
    };

    if (std::signal(SIGINT, handler) == SIG_ERR) {
        return 1;
    }

    rain_net::Server server {on_client_connected, on_client_disconnected, on_log};

    try {
        server.start(6001);

        while (running) {
            server.accept_connections();

            while (server.available_messages()) {
                const auto& [message, connection] {server.next_message()};

                handle_message(server, message, connection);
            }
        }
    } catch (const rain_net::ConnectionError& e) {
        return 1;
    }

    server.stop();

    return 0;
}
