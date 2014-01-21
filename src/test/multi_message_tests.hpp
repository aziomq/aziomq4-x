/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef __AZIOMQ_TEST_MULTIPLE_MESSAGE_HPP__
#define __AZIOMQ_TEST_MULTIPLE_MESSAGE_HPP__

#include "logger.hpp"
#include "common.hpp"

#include "../aziomq/error.hpp"
#include "../aziomq/io_service.hpp"
#include "../aziomq/option.hpp"
#include "../aziomq/socket.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/format.hpp>

#include <string>
#include <array>
#include <vector>
#include <exception>
#include <algorithm>

namespace multi_message_tests {
namespace detail {
    using const_buf_vec = std::vector<boost::asio::const_buffer>;
    using mutable_buf_vec = std::vector<boost::asio::mutable_buffer>;
    using buf_t = std::array<char, 256>;
    using buf_vec_t = std::vector<buf_t>;

    void zero(buf_vec_t & bufs) {
        for (auto& buf : bufs) {
            buf.fill(0);
        }
    }

    template<typename BufferVector>
    void init(BufferVector & buf_vec, buf_vec_t & bufs) {
        for (auto& buf : bufs) {
            buf_vec.emplace_back(boost::asio::buffer(buf));
        }
    }

    template<typename BufferVector>
    void init(BufferVector & buf_vec, const buf_vec_t & bufs) {
        for (auto& buf : bufs) {
            buf_vec.emplace_back(boost::asio::buffer(buf));
        }
    }

    template<typename I1, typename I2>
    std::exception_ptr check_res(I1 begin, I1 end, I2 expected) {
        try {
            std::for_each(begin, end, [&](decltype(*end) p) {
                    auto b = std::string(boost::asio::buffer_cast<const char*>(p));
                    auto e = std::string(boost::asio::buffer_cast<const char*>(*expected++));
                    if (b != e) {
                    std::ostringstream stm;
                    stm << boost::format("Expecting %1%, got %2%") % e % b;
                    throw std::runtime_error(stm.str());
                    }
                    });
        } catch (...) {
            return std::current_exception();
        }
        return std::exception_ptr();
    }

    std::exception_ptr check_res(const mutable_buf_vec & bufs, const const_buf_vec & expected_bufs) {
        auto expected_len = std::distance(std::begin(expected_bufs), std::end(expected_bufs));
        auto actual_len = std::distance(std::begin(bufs), std::end(bufs));
        if (expected_len != actual_len) {
            std::ostringstream stm;
            stm << boost::format("Expecting %1% buffers, got %2% buffers") % expected_len % actual_len;
            return std::make_exception_ptr(std::runtime_error(stm.str()));
        }
        return check_res(std::begin(bufs), std::end(bufs), std::begin(expected_bufs));
    }

    void fsend_sync(boost::asio::io_service &, aziomq::socket & socket, const const_buf_vec & bufs, int flags) {
        SYNC_LOG(__PRETTY_FUNCTION__);
        socket.send(bufs, flags);
    }

    void fsend_async(boost::asio::io_service & ios, aziomq::socket & socket, const const_buf_vec & bufs, int flags) {
        SYNC_LOG(__PRETTY_FUNCTION__);
        std::exception_ptr err;
        socket.async_send(bufs,
                [&ios,&err](const boost::system::error_code & ec, size_t bytes_transferred) {
                    if (ec)
                        err = std::make_exception_ptr(boost::system::system_error(ec));
                    ios.stop();
                });
        ios.run();
        if (err != std::exception_ptr())
            std::rethrow_exception(err);
    }

    void freceive_sync(boost::asio::io_service &, aziomq::socket & socket,
                            const const_buf_vec & expected_bufs, int flags) {
        SYNC_LOG(__PRETTY_FUNCTION__);
        // create vector of raw bufs to fill from length of expected_bufs
        buf_vec_t buf_vec(std::distance(std::begin(expected_bufs),
                                        std::end(expected_bufs)));
        zero(buf_vec);

        // create azio buffer vector
        mutable_buf_vec bufs;
        init(bufs, buf_vec);

        socket.receive(bufs, flags);
        auto e = check_res(bufs, expected_bufs);
        if (e != std::exception_ptr())
            std::rethrow_exception(e);
    }

    void freceive_async(boost::asio::io_service & ios, aziomq::socket & socket,
                            const const_buf_vec & expected_bufs, int flags) {
        SYNC_LOG(__PRETTY_FUNCTION__);
         //create vector of raw bufs to fill from length of expected_bufs
        buf_vec_t buf_vec(std::distance(std::begin(expected_bufs),
                                        std::end(expected_bufs)));
        zero(buf_vec);

         //create azio buffer vector
        mutable_buf_vec bufs;
        init(bufs, buf_vec);

        std::exception_ptr err;
        socket.async_receive(bufs,
                [&ios, &bufs, &err, expected_bufs](const boost::system::error_code & ec, size_t bytes_transferred) {
                    if (ec) {
                        err = std::make_exception_ptr(boost::system::system_error(ec));
                    } else {
                        err = check_res(bufs, expected_bufs);
                    }
                    ios.stop();
                });
        ios.run();
        if (err != std::exception_ptr())
            std::rethrow_exception(err);
    }

    buf_vec_t get_test_data(const std::string & msg, int msg_ct) {
        buf_vec_t res;
        for (auto i = 0; i < msg_ct; i++) {
            std::ostringstream stm;
            stm << boost::format("%1% - %2%") % msg % i;
            auto s = stm.str();

            detail::buf_t buf;
            buf.fill(0);
            std::copy(std::begin(s), std::end(s), buf.data());
            res.emplace_back(buf);
        }
        return std::move(res);
    }
} // namespace detail

void apply(std::string msg = "This is a test", int msg_ct = 20) {
    using namespace detail;
    auto buf_data = get_test_data(msg, msg_ct);
    const_buf_vec bufs;
    init(bufs, buf_data);
    SYNC_LOG("Testing Multiple Message send/receive");
    SYNC_LOG(" - Testing synchronous send/receive");
    apply_test(freceive_sync, fsend_sync, bufs);

    SYNC_LOG("Testing Multi-Part Message send/receive");
    SYNC_LOG(" - Testing synchronous send/receive");
    apply_test(freceive_sync, fsend_sync, bufs, ZMQ_SNDMORE, ZMQ_RCVMORE);

    SYNC_LOG(" - Testing aynchronous send/receive");
    SYNC_LOG(" - Testing aynchronous send/receive");
    apply_test(detail::freceive_async, detail::fsend_async, bufs);
}

} // namespace multi_message_tests
#endif // __AZIOMQ_TEST_MULTIPLE_MESSAGE_HPP__
