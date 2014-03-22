//  Reading from multiple sockets in C++
//  using aziomq async socket handlers
#include <aziomq/socket.hpp>
#include <boost/asio.hpp>

#include <array>

namespace asio = boost::asio;

int main(int argc, char **argv) {
    asio::io_service ios;

    // Register signal handler to stop the io_service
    asio::signal_set signals(ios, SIGINT, SIGTERM);
    signals.async_wait([&](const boost::system::error_code &, int) { ios.stop(); });

    // Connect to task ventilator
    aziomq::socket receiver(ios, ZMQ_PULL);
    receiver.connect("tcp://localhost:5556");

    // schedule receiver socket
    std::array<char, 4096> task;
    receiver.async_receive(asio::buffer(task), [&](const boost::system::error_code & ec,
                                                   size_t bytes_transferred) {
            if (!ec) {
                // process task
            }
    });

    // Connect to weather server
    aziomq::socket subscriber(ios, ZMQ_SUB);
    subscriber.connect("tcp://localhost:5556");
    subscriber.set_option(aziomq::socket::subscribe("10001 "));

    // schedule weather updates
    std::array<char, 4096> update;
    subscriber.async_receive(asio::buffer(task), [&](const boost::system::error_code & ec,
                                                     size_t bytes_transferred) {
            if (!ec) {
                // process weather update
            }
    });
    ios.run();
    return 0;
}
