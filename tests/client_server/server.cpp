#include <iostream>
#include <cstdint>

#include <rain_net/server.hpp>

enum class MsgType : std::uint16_t {
    PingServer
};

class ThisServer : public rain_net::Server {
public:
    ThisServer(std::uint16_t port)
        : rain_net::Server(port) {}

    virtual bool on_client_connected(std::shared_ptr<rain_net::Connection> client_connection) override {
        std::cout << "Yaay... " << client_connection->get_id() << '\n';

        return true;
    }

    virtual void on_client_disconnected(std::shared_ptr<rain_net::Connection> client_connection) override {
        std::cout << "Removed client " << client_connection->get_id() << '\n';
    }

    virtual void on_message_received(std::shared_ptr<rain_net::Connection> client_connection, rain_net::Message& message) override {
        switch (message.id()) {
            case rain_net::id(MsgType::PingServer): {
                std::cout << "Ping request from " << client_connection->get_id() << '\n';

                // Just send the same message back
                send_message(client_connection, message);

                break;
            }
        }
    }
};

#ifdef __linux__

#include <cstdlib>
#include <signal.h>

static volatile bool running = true;

int main() {
    struct sigaction sa {};

    sa.sa_handler = [](int) {
        running = false;
    };

    if (sigaction(SIGINT, &sa, nullptr) < 0) {
        std::abort();
    }

    ThisServer server {6001};
    server.start();

    while (running) {
        server.update(ThisServer::MAX_MSG, true);
        std::cout << "Returned from update\n";
    }

    server.stop();
}

#else

int main() {
    ThisServer server {6001};
    server.start();

    while (true) {
        server.update(ThisServer::MAX_MSG, true);
    }

    // FIXME should stop it gracefully
    server.stop();
}

#endif
