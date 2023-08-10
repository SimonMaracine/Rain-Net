#include <iostream>
#include <cstdint>

#include <rain_net/server.hpp>

enum class MsgType {
    PingServer
};

class ThisServer : public rain_net::Server<MsgType> {
public:
    ThisServer(std::uint16_t port)
        : rain_net::Server<MsgType>(port) {}

    virtual bool on_client_connected(std::shared_ptr<rain_net::Connection<MsgType>> client_connection) override {
        std::cout << "Yaay... " << client_connection->get_id() << '\n';

        return true;
    }

    virtual void on_client_disconnected(std::shared_ptr<rain_net::Connection<MsgType>> client_connection) override {
        std::cout << "Removed client " << client_connection->get_id() << '\n';
    }

    virtual void on_message_received(std::shared_ptr<rain_net::Connection<MsgType>> client_connection, rain_net::Message<MsgType>& message) override {
        switch (message.id()) {
            case MsgType::PingServer: {
                std::cout << "Ping request from " << client_connection->get_id() << '\n';

                // Just send the same message back
                send_message(client_connection, message);

                break;
            }
        }
    }
};

int main() {
    ThisServer server {6001};
    server.start();

    while (true) {
        server.update(ThisServer::MAX, true);
    }

    server.stop();
}
