/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef __AZIOMQ_TEST_SINGLE_MESSAGE_HPP__
#define __AZIOMQ_TEST_SINGLE_MESSAGE_HPP__

#include "logger.hpp"

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
    void fsend_sync(boost::asio::io_service &, aziomq::socket & socket, std::string msg) {
        SYNC_LOG(__PRETTY_FUNCTION__);
        socket.send(boost::asio::buffer(msg));
    }

    void fsend_async(boost::asio::io_service & ios, aziomq::socket & socket, std::string msg) {
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

    std::exception_ptr check_res(const std::string & msg, const std::string & expected) {
        if (msg != expected) {
            std::ostringstream stm;
            stm << "Expecting " << expected << ", got " << msg;
            return std::make_exception_ptr(std::runtime_error(stm.str()));
        }
        return std::exception_ptr();
    }

    void freceive_sync(boost::asio::io_service &, aziomq::socket & socket, std::string expected) {
        SYNC_LOG(__PRETTY_FUNCTION__);
        std::array<char, 256> buf;
        buf.fill(0);

        socket.receive(boost::asio::buffer(buf));
        auto e = check_res(std::string{ buf.data() }, expected);
        if (e != std::exception_ptr())
            std::rethrow_exception(e);
    }

    void freceive_async(boost::asio::io_service & ios, aziomq::socket & socket, std::string expected) {
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

    struct apply_concept {
        virtual ~apply_concept() { }
        virtual void apply(boost::asio::io_service & ios, aziomq::socket & socket, std::string msg) = 0;
    };

    template<typename F>
    struct apply_model : apply_concept {
        apply_model(F func) : data(std::move(func)) { }
        virtual void apply(boost::asio::io_service & ios, aziomq::socket & socket, std::string msg) {
            data(ios, socket, std::move(msg));
        }

        F data;
    };

    template<typename F1,
            typename F2>
    struct apply_test {
        using ptr_t = std::shared_ptr<apply_concept>;

        apply_test(F1 func1, F2 func2) :
            f1(std::make_shared<apply_model<F1>>(std::move(func1))),
            f2(std::make_shared<apply_model<F2>>(std::move(func2))) { }

        void apply(std::string uri, std::string msg) {
            std::thread t1(do_receiver, f1, uri, msg);
            std::thread t2(do_sender, f2, uri, msg);
            t2.join();
            t1.join();
        }

        static void do_sender(ptr_t p, std::string uri, std::string msg) {
            boost::asio::io_service io_service;
            aziomq::socket socket(io_service, ZMQ_PAIR);
            socket.connect(uri);
            p->apply(io_service, socket, std::move(msg));
        }

        static void do_receiver(ptr_t p, std::string uri, std::string expected) {
            boost::asio::io_service io_service;
            aziomq::socket socket(io_service, ZMQ_PAIR);
            socket.bind(uri);
            p->apply(io_service, socket, std::move(expected));
        }

        ptr_t f1;
        ptr_t f2;
    };

    template<typename F1,
            typename F2>
    void apply(F1 func1, F2 func2) {
        std::string uri{ "inproc://testing" };
        std::string msg{ "This is a test" };
        apply_test<F1, F2> f(std::move(func1), std::move(func2));
        f.apply(uri, msg);
    }
} // namespace detail

void apply() {
    SYNC_LOG("Testing Single Message send/receive");
    SYNC_LOG(" - Testing synchronous send/receive");
    detail::apply(detail::freceive_sync, detail::fsend_sync);

    SYNC_LOG(" - Testing aynchronous send/receive");
    detail::apply(detail::freceive_async, detail::fsend_async);
}
} // namespace single_message_tests
#endif // __AZIOMQ_TEST_SINGLE_MESSAGE_HPP__
