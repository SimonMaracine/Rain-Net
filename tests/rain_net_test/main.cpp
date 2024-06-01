#include <iostream>
#include <string>
#include <utility>
#include <cstdint>

#include <rain_net/server.hpp>
#include <rain_net/client.hpp>
#include <rain_net/version.hpp>

enum Foo : std::uint16_t {
    One,
    Two
};

int main() {
    rain_net::Message message {Foo::Two};

    message << 1 << 2 << 3;

    int a, b, c;

    message >> c >> b >> a;

    std::cout << a << ' ' << b << ' ' << c << '\n';

    std::cout << message.id() << ", " << message.size() << '\n';

    asio::io_context ctx;
    rain_net::internal::SyncQueue<rain_net::internal::OwnedMsg> q1;
    rain_net::internal::SyncQueue<rain_net::Message> q2;

    rain_net::ClientConnection* connection {
        new rain_net::ClientConnection(ctx, asio::ip::tcp::socket(ctx), q1, 0, {})
    };

    delete connection;

    asio::ip::tcp::resolver resolver {ctx};
    auto endpoints {resolver.resolve("localhost", "12345")};

    rain_net::ServerConnection* connection2 {
        new rain_net::ServerConnection(ctx, asio::ip::tcp::socket(ctx), q2, endpoints)
    };

    delete connection2;

    std::cout << rain_net::VERSION_MINOR << '\n';
}
