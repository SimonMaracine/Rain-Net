#include <chrono>
#include <iostream>

#include <rain_net/client.hpp>

enum class MsgType {
    PingServer
};

class ThisClient : public rain_net::Client<MsgType> {
public:
    void ping_server() {
        auto message = rain_net::new_message(MsgType::PingServer, 8);

        auto current_time = std::chrono::system_clock::now();

        message << current_time;

        send(message);
    }
};

int main() {
    ThisClient client;

    if (!client.connect("localhost", 6008)) {
        return 1;
    }

    std::cout << "ENTER LOOP\n";

    while (true) {
        if (!client.is_connected()) {
            std::cout << "DISCONNECTED\n";

            break;
        } else {
            static bool once = true;

            if (once) {
                std::cout << "CONNECTED!!!\n";
                // client.ping_server();
            }

            once = false;
        }

        if (!client.incoming_messages.empty()) {
            auto message = client.incoming_messages.pop_front().msg;

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
