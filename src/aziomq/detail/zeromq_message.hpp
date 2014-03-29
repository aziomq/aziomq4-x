/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_ZEROMQ_MESSAGE_HPP_
#define AZIOMQ_ZEROMQ_MESSAGE_HPP_

#include "../error.hpp"
#include "expected.hpp"

#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/asio/buffer.hpp>

#include <zmq.h>

namespace aziomq { namespace detail {
    struct message : zmq_msg_t {
        using flags_t = int;

        message() {
            rebuild(false);
        }

        explicit message(size_t size) {
            rebuild(size, false);
        }

        message(const message & rhs) {
            auto rc = zmq_msg_copy(this, const_cast<message*>(&rhs));
            if (rc)
                throw boost::system::system_error(make_error_code());
        }

        message(message && rhs) {
            auto rc = zmq_msg_move(this, &rhs);
            if (rc)
                throw boost::system::system_error(make_error_code());
        }

        ~message() { zmq_msg_close(this); }

        message & operator=(const message & rhs) {
            auto rc = zmq_msg_copy(this, const_cast<message*>(&rhs));
            if (rc)
                throw boost::system::system_error(make_error_code());
            return *this;
        }

        message & operator=(message && rhs) {
            auto rc = zmq_msg_move(this, &rhs);
            if (rc)
                throw boost::system::system_error(make_error_code());
            return *this;
        }

        bool more() const {
            return zmq_msg_more(const_cast<message*>(this));
        }

        size_t size() const {
            return zmq_msg_size(const_cast<message*>(this));
        }

        const void* data() const {
            return zmq_msg_data(const_cast<message*>(this));
        }

        void* data() {
            return zmq_msg_data(this);
        }

        void rebuild(bool should_close = true) {
            if (should_close)
                close();
            auto rc = zmq_msg_init(this);
            if (rc)
                throw boost::system::system_error(make_error_code());
        }

        void rebuild(size_t size, bool should_close = true) {
            if (should_close)
                close();
            auto rc = zmq_msg_init_size(this, size);
            if (rc)
                throw boost::system::system_error(make_error_code());
        }

    private:
        void close() {
            auto rc = zmq_msg_close(this);
            if (rc)
                throw boost::system::system_error(make_error_code());
        }
    };

    boost::asio::const_buffers_1 buffer(const message & msg) {
        BOOST_ASSERT_MSG(msg.data() != nullptr, "Invalid message");

        return boost::asio::buffer(msg.data(), msg.size());
    }

    boost::asio::mutable_buffers_1 buffer(message & msg) {
        BOOST_ASSERT_MSG(msg.data() != nullptr, "Invalid message");

        return boost::asio::buffer(msg.data(), msg.size());
    }
} }
#endif // AZIOMQ_ZEROMQ_MESSAGE_HPP_
