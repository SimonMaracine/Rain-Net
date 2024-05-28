#include <iostream>
#include <cstdint>
#include <chrono>

#include <rain_net/client.hpp>

enum MsgType : std::uint16_t {
    PingServer
};

struct ThisClient : public rain_net::Client {
    void ping_server() {
        rain_net::Message message {MsgType::PingServer};

        // auto message {rain_net::message(rain_net::id(MsgType::PingServer), 8)};

        auto current_time {std::chrono::system_clock::now()};

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
        std::cout << client.is_connected() << '\n';
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        client.ping_server();

        while (true) {
            const auto result {client.next_incoming_message()};

            if (!result.has_value()) {
                break;
            }

            const rain_net::Message& message {*result};

            switch (message.id()) {
                case MsgType::PingServer: {
                    auto current_time {std::chrono::system_clock::now()};
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
