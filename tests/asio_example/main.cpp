#include <iostream>
#include <thread>
#include <vector>
#include <cassert>

// Should include asio

// static std::vector<char> buffer (16 * 1024);

// void task_grab_some_data(asio::ip::tcp::socket& socket) {
//     socket.async_read_some(asio::buffer(buffer.data(), buffer.size()),
//         [&](asio::error_code ec, size_t size) {
//             if (ec) {
//                 std::cout << "[ERROR]: " << ec.message() << '\n';
//                 return;
//             }

//             std::cout << "\n\nRead " << size << " bytes!\n\n";

//             for (size_t i = 0; i < size; i++) {
//                 std::cout << buffer[i];
//             }

//             task_grab_some_data(socket);
//         }
//     );
// }

// void check_error(const asio::error_code& ec) {
//     if (ec) {
//         std::cout << "[ERROR]: " << ec.message() << '\n';
//         std::exit(1);
//     }
// }

int main() {
    // asio::error_code ec;

    // // Create the context, give it some dummy tasks to do and start it
    // asio::io_context context;
    // asio::io_context::work idle_work {context};
    // std::thread context_thread {[&context]() { context.run(); }};

    // // asio::ip::tcp::resolver resolver {context};
    // // resolver.resolve("https://simonmaracine.github.io/", "80");

    // // Create an endpoint to connect to
    // asio::ip::tcp::endpoint endpoint {asio::ip::make_address("51.38.81.49", ec), 80};  // My website 185.199.108.153

    // check_error(ec);

    // // Create a socket and connect it to the website
    // asio::ip::tcp::socket socket {context};
    // socket.connect(endpoint, ec);

    // check_error(ec);

    // std::cout << "Connected!\n";

    // assert(socket.is_open());

    // // Try to write to the socket
    // std::string request = (
    //     "GET /index.html HTTP/1.1\r\n"
    //     "Host: simonmaracine.github.io\r\n"
    //     "Connection: close\r\n\r\n"
    // );
    // socket.write_some(asio::buffer(request.data(), request.size()), ec);

    // check_error(ec);

    // // // Wait until there is some bytes available
    // // socket.wait(socket.wait_read, ec);

    // // check_error(ec);

    // // // Ask if there are any bytes available to read without blocking
    // // const size_t bytes = socket.available(ec);

    // // check_error(ec);

    // // std::cout << "Bytes available: " << bytes << '\n';

    // // Read from the socket asynchronously
    // task_grab_some_data(socket);

    // // Block main thread, then exit
    // std::cin.get();

    // context.stop();
    // context_thread.join();
}
