#include <aziomq/socket.hpp>
#include <boost/asio.hpp>
#include <array>

namespace asio = boost::asio;

int main(int argc, char** argv) {
    asio::io_service ios;

    // Register signal handler to stop the io_service
    asio::signal_set signals(ios, SIGINT, SIGTERM);
    signals.async_wait([&](const boost::system::error_code &, int) { ios.stop(); });

    aziomq::socket subscriber(ios, ZMQ_SUB);
    subscriber.connect("tcp://192.168.55.112:5556");
    subscriber.connect("tcp://192.168.55.201:7721");
    subscriber.set_option(aziomq::socket::subscribe("NASDAQ"));

    aziomq::socket publisher(ios, ZMQ_PUB);
    publisher.bind("ipc://nasdaq-feed");

    aziomq::socket::proxy(subscriber, publisher);
    ios.run();
    return 0;
}
