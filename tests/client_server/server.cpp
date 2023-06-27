#include <iostream>

#include <rain_net/server.hpp>

enum class MsgType {
    PingServer
};

class ThisServer : public rain_net::Server<MsgType> {
public:
    ThisServer(uint16_t port)
        : rain_net::Server<MsgType>(port) {}

    virtual bool on_client_connected(std::shared_ptr<rain_net::Connection<MsgType>> client_connection) override {
        return true;
    }

    virtual void on_client_disconnected(std::shared_ptr<rain_net::Connection<MsgType>> client_connection) override {

    }

    virtual void on_message_received(std::shared_ptr<rain_net::Connection<MsgType>> client_connection, rain_net::Message<MsgType>& message) override {
        switch (message.id()) {
            case MsgType::PingServer: {
                std::cout << "Ping request from " << client_connection->get_id() << '\n';

                // Just send the same message back
                client_connection->send(message);

                break;
            }
        }
    }
};

int main() {
    ThisServer server {6008};
    server.start();

    while (true) {
        server.update();
    }

    server.stop();
}
