#include <aziomq/socket.hpp>
#include <aziomq/thread.hpp>
#include <boost/asio.hpp>

#include <array>
#include <string>
#include <iostream>

namespace asio = boost::asio;

struct ping_handler {
    asio::io_service & ios_;
    aziomq::socket & s_;

    using buffer_type = std::array<char, 1024>;
    buffer_type & buf_;

    ping_handler(asio::io_service & ios, aziomq::socket & s, buffer_type & buf)
        : ios_(ios)
        , s_(s)
        , buf_(buf)
    { }

    void operator()(boost::system::error_code const& ec, size_t) {
        if (ec) {
            std::cout << "ping_handler error " << ec << std::endl;
            return;
        }

        std::string req(buf_.data());
        if (req != "PING!") {
            s_.send(asio::buffer("BYE!"));
            ios_.stop();
            return;
        }
        s_.send(asio::buffer("PONG!"));
        async_receive();
    }

    void async_receive() {
        std::fill(std::begin(buf_), std::end(buf_), 0);
        s_.async_receive(asio::buffer(buf_), *this);
    }
};

int main(int argc, char** argv) {
    asio::io_service ios;

    auto server = aziomq::thread::fork(ios, [](aziomq::socket client, std::string howdy) {
        client.send(asio::buffer(howdy));

        auto& ios = client.get_io_service();
        ping_handler::buffer_type buf;
        ping_handler handler(ios, client, buf);
        handler.async_receive();

        ios.run();
        std::cout << "Server thread exiting" << std::endl;
    },
    "HOWDY");

    ping_handler::buffer_type buf;
    auto bytes_transferred = server.receive(asio::buffer(buf));
    std::cout << "Server says - " << std::string(buf.data(), bytes_transferred) << std::endl;

    auto ping_for = 10;
    for (int i = 0; i <= ping_for; i++) {
        if (i == ping_for)
            server.send(asio::buffer("HASTA LA VISTA, BABY!"));
        else
            server.send(asio::buffer("PING!"));

        bytes_transferred = server.receive(asio::buffer(buf));
        std::cout << "Server says - " << std::string(buf.data(), bytes_transferred) << std::endl;
    }
}
