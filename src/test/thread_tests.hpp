/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef __AZIOMQ_TEST_THREAD_HPP__
#define __AZIOMQ_TEST_THREAD_HPP__
#include "logger.hpp"
#include "common.hpp"

#include "../aziomq/error.hpp"
#include "../aziomq/io_service.hpp"
#include "../aziomq/option.hpp"
#include "../aziomq/socket.hpp"
#include "../aziomq/thread.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/asio/buffer.hpp>

#include <array>
#include <string>
#include <exception>
#include <mutex>
#include <condition_variable>

namespace thread_tests {
namespace detail {
    std::exception_ptr check_res(const std::string & msg, const std::string & expected) {
        if (msg != expected) {
            std::ostringstream stm;
            stm << "Expecting " << expected << ", got " << msg;
            return std::make_exception_ptr(std::runtime_error(stm.str()));
        }
        return std::exception_ptr();
    }
} // namespace detail

void apply() {
    std::string test_str("Testing");
    boost::asio::io_service ios;
    using lock_t = std::unique_lock<std::mutex>;
    lock_t::mutex_type mtx;
    std::condition_variable cv;
    bool finished = false;

    {
        auto s = aziomq::thread::fork(ios, [&](aziomq::socket p, std::string msg) {
            p.send(boost::asio::buffer(msg));
            auto& ios = p.get_io_service();
            ios.run();
            {
                lock_t lk(mtx);
                finished = true;
            }
            cv.notify_one();
        }, test_str);

        std::array<char, 1025> buf;
        auto bytes_transferred = s.receive(boost::asio::buffer(buf));
        auto res = detail::check_res(std::string(buf.data(), bytes_transferred), test_str);
        if (res != std::exception_ptr())
            std::rethrow_exception(res);
    }

    {
        lock_t lk(mtx);
        cv.wait(lk, [&] { return finished; });
    }
}

} // namespace thread_tests
#endif // __AZIOMQ_TEST_THREAD_HPP__

