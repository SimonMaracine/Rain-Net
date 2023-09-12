#include <iostream>
#include <string>
#include <utility>
#include <cstdint>

#include <rain_net/message.hpp>
#include <rain_net/connection.hpp>
#include <rain_net/server.hpp>
#include <rain_net/client.hpp>
#include <rain_net/queue.hpp>
#include <rain_net/version.hpp>

enum class Foo : std::uint16_t {
    One,
    Two
};

int main() {
    rain_net::Message message = rain_net::message(rain_net::id(Foo::Two), 20);

    message << 1 << 2 << 3;

    int a, b, c;

    message >> c >> b >> a;

    std::cout << a << ' ' << b << ' ' << c << '\n';

    std::cout << message << '\n';

    asio::io_context ctx;
    rain_net::internal::Queue<rain_net::internal::OwnedMsg> q;

    [[maybe_unused]] rain_net::Connection* connection = (
        new rain_net::internal::ClientConnection(&ctx, &q, asio::ip::tcp::socket(ctx), 0)
    );

    delete connection;

    asio::ip::tcp::resolver resolver {ctx};
    auto endpoints = resolver.resolve("localhost", "12345");

    [[maybe_unused]] rain_net::Connection* connection2 = (
        new rain_net::internal::ServerConnection(&ctx, &q, asio::ip::tcp::socket(ctx), endpoints, []() {})
    );

    delete connection2;

    std::cout << rain_net::VERSION_MINOR << '\n';
}
