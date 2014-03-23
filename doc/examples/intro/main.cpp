#include <aziomq/socket.hpp>
#include <boost/asio.hpp>
#include <array>

namespace asio = boost::asio;

int main(int argc, char** argv) {
    asio::io_service ios;
    aziomq::socket subscriber(ios, ZMQ_SUB);
    subscriber.connect("tcp://192.168.55.112:5556");
    subscriber.connect("tcp://192.168.55.201:7721");
    subscriber.set_option(aziomq::socket::subscribe("NASDAQ"));

    aziomq::socket publisher(ios, ZMQ_PUB);
    publisher.bind("ipc://nasdaq-feed");

    std::array<char, 256> buf;
    for (;;) {
        auto size = subscriber.receive(asio::buffer(buf));
        publisher.send(asio::buffer(const_cast<const char*>(buf.data()), size));
    }
    return 0;
}
