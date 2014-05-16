#include <aziomq/socket.hpp>
#include <boost/asio.hpp>
#include <array>
#include <thread>
#include <iostream>

namespace asio = boost::asio;

boost::format what(zmq_event_t const& event) {
    switch (event.event) {
        case ZMQ_EVENT_LISTENING:
            return boost::format("listening socket descriptor %1%\n"
                                 "listening socket address %2%");
        case ZMQ_EVENT_ACCEPTED:
            return boost::format("accepted socket descriptor %1%\n"
                                 "accepted socket address %2%");
        case ZMQ_EVENT_CLOSE_FAILED:
            return boost::format("socket close failure error code %1%\n"
                                 "socket address %s");
        case ZMQ_EVENT_CLOSED:
            return boost::format("closed socket descriptor %1%\n"
                                 "closed socket address %2%");
        case ZMQ_EVENT_DISCONNECTED:
            return boost::format("disconnected socket descriptor %1%\n"
                                 "disconnected socket address %2%");
        default:
            return boost::format("unknown event %1% on socket descriptor %2%\n"
                                 "socket address %3%") % event.event;
    }
}

void rep_socket_monitor(boost::system::error_code const& ec,
                        zmq_event_t const& event,
                        boost::asio::const_buffer const& addr) {
    auto b = asio::buffer_cast<char const*>(addr);
    auto e = b + asio::buffer_size(addr);
    std::cout << what(event) % event.value % std::string(b, e) << std::endl;
}

int main(int argc, char** argv) {
    asio::io_service ios;
    asio::io_service ios_mon;

    // Register signal handler to stop the io_service
    asio::signal_set signals(ios, SIGINT, SIGTERM);
    signals.async_wait([&](const boost::system::error_code &, int) { ios.stop(); });

    aziomq::socket rep(ios, ZMQ_REP);
    auto mon = rep.monitor(ios_mon, rep_socket_monitor);
    std::thread([&ios_mon]() { ios_mon.run(); });
    ios.run();
    ios_mon.stop();
    return 0;
}
