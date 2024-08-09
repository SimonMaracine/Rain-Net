#include <iostream>
#include <csignal>
#include <chrono>

#include <rain_net/client.hpp>

enum MessageType {
    PingServer
};

void handle_message(const rain_net::Message& message) {
    switch (message.id()) {
        case MessageType::PingServer: {
            const auto current_time {std::chrono::steady_clock::now()};
            std::chrono::steady_clock::time_point previous_time;

            rain_net::MessageReader reader;
            reader(message) >> previous_time;

            std::cout << "Ping: " << std::chrono::duration<double>(current_time - previous_time).count() << '\n';

            break;
        }
    }
}

void ping_server(rain_net::Client& client) {
    rain_net::Message message {MessageType::PingServer};

    const auto current_time {std::chrono::steady_clock::now()};
    message << current_time;

    client.send_message(message);
}

static volatile bool running {true};

int main() {
    const auto handler {
        [](int) { running = false; }
    };

    if (std::signal(SIGINT, handler) == SIG_ERR) {
        return 1;
    }

    rain_net::Client client;

    try {
        client.connect("localhost", 6001);

        while (!client.connection_established()) {
            if (!running) {
                return 0;
            }
        }

        std::cout << "Connected\n";

        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(40));

            ping_server(client);

            while (client.available()) {
                const auto message {client.next_message()};

                handle_message(message);
            }
        }
    } catch (const rain_net::ConnectionError& e) {
        std::cout << e.what() << '\n';
        return 1;
    }

    client.disconnect();

    return 0;
}
