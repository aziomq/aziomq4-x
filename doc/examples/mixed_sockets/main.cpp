//  Reading from multiple sockets in C++
//  using aziomq async socket handlers
#include <aziomq/socket.hpp>
#include <boost/asio.hpp>
#include <boost/system/system_error.hpp>

#include <array>
#include <algorithm>

namespace asio = boost::asio;


struct translator {
    asio::ip::udp::socket & frontend;
    aziomq::socket & backend;
    translator(asio::ip::udp::socket & f,
               aziomq::socket & b,
               const asio::ip::udp::endpoint & group,
               const std::string & topic_uri,
               const asio::ip::address_v4 & listen = asio::ip::address_v4()) :
        frontend(f), backend(b) {
            frontend.open(group.protocol());
            frontend.set_option(asio::ip::udp::socket::reuse_address(true));
            frontend.bind(asio::ip::udp::endpoint(listen, group.port()));
            frontend.set_option(asio::ip::multicast::join_group(listen, group.address().to_v4()));

            backend.bind(topic_uri);
        }

    void operator()(const boost::system::error_code & ec = boost::system::error_code(),
                    size_t bytes_transferred = 0) {
            if (ec)
                return;
            if (bytes_transferred) {
                auto bufsize = translate(exch_buf.data(), pub_buf.data(), bytes_transferred);
                backend.send(asio::buffer(const_cast<const char*>(pub_buf.data()), bufsize));
            }
            frontend.async_receive(asio::buffer(exch_buf), *this);
    }

private:
    std::array<char, 4096> exch_buf;
    std::array<char, 4096> pub_buf;
    size_t translate(const char* exch_buf, char* res_buf, size_t packet_size) {
        // normalize external representation
        // ...
        std::copy_n(exch_buf, packet_size, res_buf);
        return packet_size;
    }
};

int main(int argc, char **argv) {
    asio::io_service ios;

    // to change context options you must do so before any sockets are created
    aziomq::io_service::set_option(ios, aziomq::io_service::io_threads(2));

    // run until sigterm or sigint
    asio::signal_set signals(ios, SIGINT, SIGTERM);
    signals.async_wait([&](const boost::system::error_code &, int) { ios.stop(); });

    asio::ip::udp::endpoint group;
    group.address(asio::ip::address::from_string("242.1.1.1"));
    group.port(42424);
    asio::ip::udp::socket listen(ios);
    aziomq::socket publish(ios, ZMQ_PUB);

    // register the translator on listen/publish sockets
    translator(listen, publish, group, "ipc://nasdaq-feed")();

    // spawn threads to handle inproc messaging
    // ...
    ios.run();
    return 0;
}
