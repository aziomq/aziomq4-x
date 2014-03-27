/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef __AZIOMQ_TEST_SINGLE_MESSAGE_HPP__
#define __AZIOMQ_TEST_SINGLE_MESSAGE_HPP__

#include "logger.hpp"
#include "common.hpp"

#include "../aziomq/error.hpp"
#include "../aziomq/io_service.hpp"
#include "../aziomq/option.hpp"
#include "../aziomq/socket.hpp"

#include <boost/asio/buffer.hpp>

#include <string>
#include <exception>
#include <thread>

namespace single_message_tests {
namespace detail {
    std::exception_ptr check_res(const std::string & msg, const std::string & expected) {
        if (msg != expected) {
            std::ostringstream stm;
            stm << "Expecting " << expected << ", got " << msg;
            return std::make_exception_ptr(std::runtime_error(stm.str()));
        }
        return std::exception_ptr();
    }

    void fsend_sync(boost::asio::io_service &, aziomq::socket & socket, std::string msg, int flags) {
        SYNC_LOG(__PRETTY_FUNCTION__);
        socket.send(boost::asio::buffer(msg));
    }

    void fsend_async(boost::asio::io_service & ios, aziomq::socket & socket, std::string msg, int flags) {
        SYNC_LOG(__PRETTY_FUNCTION__);
        std::exception_ptr err;
        socket.async_send(boost::asio::buffer(msg),
                [&ios,&err](const boost::system::error_code & ec, size_t bytes_transferred) {
                    if (ec)
                        err = std::make_exception_ptr(boost::system::system_error(ec));
                    ios.stop();
                });
        ios.run();
        if (err != std::exception_ptr())
            std::rethrow_exception(err);
    }

    void freceive_sync(boost::asio::io_service &, aziomq::socket & socket, std::string expected, int flags) {
        SYNC_LOG(__PRETTY_FUNCTION__);
        std::array<char, 256> buf;
        buf.fill(0);

        socket.receive(boost::asio::buffer(buf));
        auto e = check_res(std::string{ buf.data() }, expected);
        if (e != std::exception_ptr())
            std::rethrow_exception(e);
    }

    void freceive_async(boost::asio::io_service & ios, aziomq::socket & socket, std::string expected, int flags) {
        SYNC_LOG(__PRETTY_FUNCTION__);
        std::array<char, 256> buf;
        buf.fill(0);

        std::exception_ptr err;
        socket.async_receive(boost::asio::buffer(buf),
                [&ios, &buf, &err, expected](const boost::system::error_code & ec, size_t bytes_transferred) {
                    if (ec) {
                        err = std::make_exception_ptr(boost::system::system_error(ec));
                    } else {
                        err = check_res(std::string{ buf.data() }, expected);
                    }
                    ios.stop();
                });
        ios.run();
        if (err != std::exception_ptr())
            std::rethrow_exception(err);
    }
} // namespace detail

void apply(std::string msg = "This is a test") {
    using namespace detail;
    SYNC_LOG("Testing Single Message send/receive");
    SYNC_LOG(" - Testing synchronous send/receive");
    apply_test(freceive_sync, fsend_sync, msg);

    SYNC_LOG(" - Testing aynchronous send/receive");
    apply_test(freceive_async, fsend_async, msg);
}
} // namespace single_message_tests
#endif // __AZIOMQ_TEST_SINGLE_MESSAGE_HPP__
