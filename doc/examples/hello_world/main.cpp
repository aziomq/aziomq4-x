// Hello World server in C++ using aziomq using async reactor
//  Binds REP socket to tcp://*:5555
//  Expects "Hello" from client, replies with "World"
#include <aziomq/socket.hpp>
#include <boost/asio.hpp>

#include <array>
#include <iostream>

namespace asio = boost::asio;

int main(int argc, char** argv) {
    asio::io_service ios;

    // Register signal handler to stop the io_service
    asio::signal_set signals(ios, SIGINT, SIGTERM);
    signals.async_wait([&](const boost::system::error_code &, int) { ios.stop(); });

    // Socket to talk to clients
    aziomq::socket responder(ios, ZMQ_REP);
    responder.bind("tcp://*:5555");

    std::array<char, 4096> buf;
    responder.async_receive(asio::buffer(buf), [&](const boost::system::error_code & ec,
                                                   size_t bytes_transferred) {
               if (ec)  {
                    std::cout << "Error " << ec;
                    return;
                }
                std::cout << "Received Hello" << std::endl;
                responder.send(asio::buffer("World"));
            });
    ios.run();
    return 0;
}
