#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <csignal>
#include <chrono>

#include <rain_net/client.hpp>

enum MsgType : std::uint16_t {
    PingServer
};

struct ThisClient : public rain_net::Client {
    void ping_server() {
        rain_net::Message message {MsgType::PingServer};

        const auto current_time {std::chrono::system_clock::now()};

        message << current_time;

        send_message(message);
    }

    void process_messages() {
        while (true) {
            const auto result {next_incoming_message()};

            if (!result.has_value()) {
                break;
            }

            const rain_net::Message& message {*result};

            switch (message.id()) {
                case MsgType::PingServer: {
                    const auto current_time {std::chrono::system_clock::now()};
                    std::chrono::system_clock::time_point server_time;

                    message >> server_time;

                    std::cout << "Ping: " << std::chrono::duration<double>(current_time - server_time).count() << '\n';

                    break;
                }
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

    ThisClient client;
    client.connect("localhost", 6001);

    if (client.fail()) {
        std::cout << client.fail_reason() << '\n';
        client.disconnect();
        return 1;
    }

    while (!client.is_connected()) {
        std::cout << "Not yet connected\n";

        if (!running) {
            client.disconnect();
            return 1;
        }
    }

    std::cout << "Connected\n";

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        client.ping_server();
        client.process_messages();

        if (client.fail()) {
            std::cout << "Unexpected error: " << client.fail_reason() << '\n';
            break;
        }
    }

    client.disconnect();
}
