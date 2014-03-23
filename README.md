# aziomq4-x

## Welcome
The aziomq library provides Boost Asio style bindings for ZeroMq 4.x

This library is built on top of ZeroMQ's standard C interface.

The main abstraction exposed by the library is aziomq::socket which
provides an abstraction over the underlying zeromq socket that
interfaces with Asio's io_service() and participates in the io_service's
epoll reactor.

## Building and installation

Building requires a recent version of CMake (2.8 or later), and a C++ compiler
which supports '--std=c++11'.  Currently this has been tested with - 
* OSX10.9 Mavericks XCode5.1
* Arch Linux GCC4.8

Library dependencies are -
* Boost 1.53 or later
* ZeroMQ 4.0

To build -
```
$ mkdir build && cd build
$ cmake ..
$ make
$ make test
$ make install
```

To change the default install location use -DCMAKE_INSTALL_PREFIX when invoking cmake
You can also change where the build looks for Boost and CMake by setting -

```
$ export BOOST_ROOT=<my custom Boost install>
$ export ZMQ_ROOT=<my custom ZeroMQ install>
$ mkdir build && cd build
$ cmake ..
$ make
$ ...
```

## Example Code
This is an aziomq version of the code presented in the ZeroMQ guide at
http://zeromq.org/intro:read-the-manual

```
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
```

Further examples may be found in doc/examples

## Copying

Use of this software is granted under the the BOOST 1.0 license
(same Boost Asio).  For details see the file `LICENSE-BOOST_1_0
included with the distribution.
