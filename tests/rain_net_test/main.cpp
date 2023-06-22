#include <iostream>
#include <string>
#include <utility>

#include <rain_net/message.hpp>
#include <rain_net/connection.hpp>
#include <rain_net/server.hpp>

enum class Foo {
    One,
    Two
};

int main() {
    rain_net::Message<Foo> message = rain_net::new_message(Foo::Two, 20);

    message << 1 << 2 << 3;

    int a, b, c;

    message >> c >> b >> a;

    std::cout << a << ' ' << b << ' ' << c << '\n';

    std::cout << message << '\n';

    asio::io_context ctx;
    rain_net::Queue<rain_net::OwnedMessage<Foo>> q;

    [[maybe_unused]] rain_net::Connection<Foo>* conn = new rain_net::ClientConnection<Foo>(&ctx, &q, asio::ip::tcp::socket(ctx), 0);

    asio::ip::tcp::resolver resolver {ctx};
    auto endpoints = resolver.resolve("localhost", "12345");

    [[maybe_unused]] rain_net::Connection<Foo>* conn2 = new rain_net::ServerConnection<Foo>(&ctx, &q, asio::ip::tcp::socket(ctx), std::move(endpoints));
}
