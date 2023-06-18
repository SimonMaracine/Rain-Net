#include <iostream>

#include <rain_net/message.hpp>

enum class Foo {
    One,
    Two
};

int main() {
    rain_net::Message<Foo> message;
    message.header.id = Foo::Two;

    message << 1 << 2 << 3;

    int a, b, c;

    message >> c >> b >> a;

    std::cout << a << ' ' << b << ' ' << c << '\n';

    std::cout << message << '\n';
}
