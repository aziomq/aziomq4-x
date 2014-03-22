//  Reading from multiple sockets in C++
//  using aziomq async socket handlers
#include <aziomq/socket.hpp>
#include <boost/asio.hpp>
#include <boost/system/system_error.hpp>

#include <array>

namespace asio = boost::asio;

void setup_socket(asio::ip::udp::socket & socket,
                  const asio::ip::udp::endpoint & group,
                  const asio::ip::address_v4 & listen) {
    socket.open(group.protocol());
    socket.set_option(asio::ip::udp::socket::reuse_address(true));
    socket.bind(asio::ip::udp::endpoint(listen, group.port()));
    socket.set_option(asio::ip::multicast::join_group(listen, group.address().to_v4()));
}

size_t translate(void * exch_buf, void * res_buf, size_t packet_size) {
    // normalize external representation
    // ...
    return 424;
}

int main(int argc, char **argv) {
    asio::io_service ios;

    // receive exchange traffice on udp multicast
    asio::ip::udp::socket exchange(ios);
    asio::ip::udp::endpoint group;
    group.address(asio::ip::address::from_string("242.1.1.1"));
    group.port(42424);

    setup_socket(exchange, group, asio::ip::address_v4());

    aziomq::socket publisher(ios, ZMQ_PUB);
    publisher.bind("ipc://nasdaq-feed");

    std::array<char, 4096> exch_buf;
    std::array<char, 4096> pub_buf;
    exchange.async_receive(asio::buffer(exch_buf), [&](const boost::system::error_code & ec,
                                                       size_t bytes_transferred) {
            if (ec)
                return;
            auto bufsize = translate(exch_buf.data(), pub_buf.data(), bytes_transferred);
            publisher.send(asio::buffer(const_cast<const char*>(pub_buf.data()), bufsize));
    });

    // run until sigterm or sigint
    asio::signal_set signals(ios, SIGINT, SIGTERM);
    signals.async_wait([&](const boost::system::error_code &, int) { ios.stop(); });
    ios.run();
    return 0;
}
