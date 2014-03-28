//  Sending/Receiving multi-part messages in C++
//  using aziomq
#include <aziomq/socket.hpp>
#include <boost/asio.hpp>

#include <array>
#include <string>
#include <vector>

namespace asio = boost::asio;

using buf_vec_t = std::vector<asio::const_buffer>;
struct req_type {
    int a;
    int b;
};

struct resp_type {
    int res;
    // ...
};

int main(int argc, char **argv) {
    asio::io_service ios;

    aziomq::socket request(ios, ZMQ_REQ);
    request.connect("tcp://192.168.55.112:5555");

    buf_vec_t bufs;
    bufs.emplace_back(asio::buffer("Header0"));
    bufs.emplace_back(asio::buffer("Header1"));
    req_type r = {42, 58 };
    bufs.emplace_back(asio::buffer(&r, sizeof(req_type)));

    request.send(bufs, ZMQ_SNDMORE);

    std::vector<std::string> headers;
    resp_type resp;
    for(;;) {
        std::array<char, 4096> buf;
        auto mr = request.receive_more(asio::buffer(buf));
        if (mr.second) {
            headers.emplace_back(std::string(buf.data(), mr.first));
        } else {
            resp = *reinterpret_cast<resp_type*>(buf.data());
            break;
        }
    }
    return 0;
}
