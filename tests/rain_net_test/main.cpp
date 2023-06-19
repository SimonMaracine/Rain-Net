#include <iostream>

#include <rain_net/message.hpp>
#include <rain_net/connection.hpp>

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
}
