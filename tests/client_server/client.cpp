#include <iostream>
#include <cstdint>
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

    void on_connected() override {
        std::cout << "Connected\n";
    }

    void on_message_received(const rain_net::Message& message) override {
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

    bool connected {false};
};

static volatile bool running {true};

int main() {
    const auto handler {
        [](int) { running = false; }
    };

    if (std::signal(SIGINT, handler) == SIG_ERR) {
        return 1;
    }

    ThisClient client;

    try {
        client.connect("localhost", 6001);

        // while (!client.connection_established()) {
        //     if (!running) {
        //         return 0;
        //     }
        // }

        // std::cout << "Connected\n";

        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(40));

            client.ping_server();
            client.update();
        }
    } catch (const rain_net::ConnectionError& e) {
        std::cout << e.what() << '\n';
        return 1;
    }

    client.disconnect();

    return 0;
}
