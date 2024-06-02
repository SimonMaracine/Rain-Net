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

        const auto current_time {std::chrono::steady_clock::now()};

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
                    const auto current_time {std::chrono::steady_clock::now()};
                    std::chrono::steady_clock::time_point previous_time;

                    rain_net::MessageReader reader;
                    reader(message) >> previous_time;

                    std::cout << "Ping: " << std::chrono::duration<double>(current_time - previous_time).count() << '\n';

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

    do {
        if (client.fail()) {
            std::cout << client.fail_reason() << '\n';
            return 1;
        }

        if (!running) {
            return 0;
        }

        std::cout << "Not yet connected\n";
    } while (!client.is_connected());

    std::cout << "Connected\n";

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));

        client.ping_server();
        client.process_messages();

        if (client.fail()) {
            std::cout << "Unexpected error: " << client.fail_reason() << '\n';
            return 1;
        }
    }

    client.disconnect();
}
