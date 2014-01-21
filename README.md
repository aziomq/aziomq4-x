# aziomq4-x

## Welcome
The aziomq library provides Boost Asio style bindings for ZeroMq 4.x

This library is built on top of ZeroMQ's standard C interface.

The main abstraction exposed by the library is aziomq::socket which
provides an abstraction over the underlying zmq::socket_t that
interfaces with Asio's io_service() and participates in the io_service's
poll loop.

## Building and installation

See the INSTALL file included with the distribution

## Copying

Use of this software is granted under the the BOOST 1.0 license
(same Boost Asio).  For details see the file `LICENSE-BOOST_1_0
included with the distribution.
