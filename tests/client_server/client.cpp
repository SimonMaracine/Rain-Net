#include <chrono>
#include <iostream>

#include <rain_net/client.hpp>

enum class MsgType {
    PingServer
};

class ThisClient : public rain_net::Client<MsgType> {
public:
    void ping_server() {
        auto message = rain_net::message(MsgType::PingServer, 8);

        auto current_time = std::chrono::system_clock::now();

        message << current_time;

        send_message(message);
    }
};

int main() {
    ThisClient client;

    if (!client.connect("localhost", 6001)) {
        return 1;
    }

    while (true) {
        std::cout << client.is_connected() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        client.ping_server();

        while (true) {
            auto result = client.next_incoming_message();
            rain_net::Message<MsgType> message = result.value_or(rain_net::Message<MsgType>());

            if (!result.has_value()) {
                break;
            }

            switch (message.id()) {
                case MsgType::PingServer: {
                    auto current_time = std::chrono::system_clock::now();
                    std::chrono::system_clock::time_point server_time;

                    message >> server_time;

                    std::cout << "Ping: " << std::chrono::duration<double>(current_time - server_time).count() << '\n';

                    break;
                }
            }
        }
    }

    client.disconnect();
}
