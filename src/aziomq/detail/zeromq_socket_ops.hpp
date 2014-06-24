/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_SOCKET_OPS_HPP_
#define AZIOMQ_SOCKET_OPS_HPP_

#include "../error.hpp"
#include "../util/expected.hpp"
#include "reactor_op.hpp"
#include "tracked_op.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/socket_base.hpp>

#include <zmq.h>

#include <algorithm>
#include <type_traits>

#include <string>

namespace aziomq { namespace detail {

namespace socket_ops {
    using socket_type = reactor_op::socket_type;
    using expected_size = util::expected<std::size_t>;
    using shutdown_type = boost::asio::socket_base::shutdown_type;
    using native_handle_type = boost::asio::detail::socket_type; // underlying FD
    using endpoint_type = std::string;
    using more_result = std::pair<std::size_t, bool>;
    using expected_more = util::expected<more_result>;

    struct send_more_t : std::integral_constant<int, ZMQ_SNDMORE> { };
    struct receive_more_t : std::integral_constant<int, ZMQ_RCVMORE> { };
    struct dont_wait_t : std::integral_constant<int, ZMQ_DONTWAIT> { };

    template<typename Option>
    boost::system::error_code set_option(socket_type socket, const Option & option,
                                         boost::system::error_code & ec) {
        BOOST_ASSERT_MSG(socket, "Invalid socket");

        ec = boost::system::error_code();
        auto rc = zmq_setsockopt(socket, option.name(), option.data(), option.size());
        if (rc)
            ec = make_error_code();
        return ec;
    }

    template<typename Option>
    boost::system::error_code get_option(socket_type socket,
                                         Option & option,
                                         boost::system::error_code & ec) {
        BOOST_ASSERT_MSG(socket, "Invalid socket");

        ec = boost::system::error_code();
        size_t size = option.size();
        auto rc = zmq_getsockopt(socket, option.name(), option.data(), &size);
        if (rc && errno == EINVAL) {
            option.resize(size);
            rc = zmq_getsockopt(socket, option.name(), option.data(), &size);
        }

        if (rc)
            ec = make_error_code();
        return ec;
    }

    inline
    boost::system::error_code native_handle(socket_type socket,
                                            native_handle_type & handle,
                                            boost::system::error_code & ec) {
        BOOST_ASSERT_MSG(socket, "Invalid socket");

        size_t size = sizeof(native_handle_type);
        auto rc = zmq_getsockopt(socket, ZMQ_FD, &handle, &size);
        if (rc)
            ec = make_error_code();
        return ec;
    }

    inline
    boost::system::error_code bind(socket_type socket,
                                   const endpoint_type & endpoint,
                                   boost::system::error_code & ec) {
        BOOST_ASSERT_MSG(socket, "Invalid socket");

        auto rc = zmq_bind(socket, endpoint.c_str());
        if (rc)
            ec = make_error_code();
        return ec;
    }

    inline
    boost::system::error_code connect(socket_type socket,
                                      const endpoint_type & endpoint,
                                      boost::system::error_code & ec) {
        BOOST_ASSERT_MSG(socket, "Invalid socket");

        auto rc = zmq_connect(socket, endpoint.c_str());
        if (rc)
            ec = make_error_code();
        return ec;
    }

    inline
    expected_size receive(socket_type socket,
                          message * msg,
                          message::flags_t flags) {
        BOOST_ASSERT_MSG(socket != nullptr, "Invalid socket");
        BOOST_ASSERT_MSG(msg != nullptr, "Invalid message");

        auto rc = zmq_msg_recv(msg, socket, flags);
        if (rc < 0)
            return expected_size::from_exception(boost::system::system_error(make_error_code()));
        return static_cast<std::size_t>(rc);
    }

    inline
    expected_size receive(socket_type socket,
                          message * msg,
                          message::flags_t flags,
                          dont_wait_t) {
        BOOST_ASSERT_MSG(socket != nullptr, "Invalid socket");
        BOOST_ASSERT_MSG(msg != nullptr, "Invalid message");

        auto rc = zmq_msg_recv(msg, socket, flags);
        if (rc == EAGAIN)
            return 0;
        if (rc < 0)
            return expected_size::from_exception(boost::system::system_error(make_error_code()));
        return static_cast<std::size_t>(rc);
    }

    inline
    expected_size send(socket_type socket,
                       message * msg,
                       message::flags_t flags) {
        BOOST_ASSERT_MSG(socket != nullptr, "Invalid socket");
        BOOST_ASSERT_MSG(msg != nullptr, "Invalid message");

        auto rc = zmq_msg_send(msg, socket, flags);
        if (rc < 0)
            return expected_size::from_exception(boost::system::system_error(make_error_code()));
        return static_cast<std::size_t>(rc);
    }

    inline
    expected_size send(socket_type socket,
                       message * msg,
                       message::flags_t flags,
                       dont_wait_t) {
        BOOST_ASSERT_MSG(socket != nullptr, "Invalid socket");
        BOOST_ASSERT_MSG(msg != nullptr, "Invalid message");

        auto sz = msg->size();
        auto rc = zmq_msg_send(msg, socket, flags | dont_wait_t::value);
        if (rc == EAGAIN)
            return 0;
        if (rc < 0)
            return expected_size::from_exception(boost::system::system_error(make_error_code()));
        return static_cast<std::size_t>(rc);
    }

    inline
    expected_size send(message & msg,
                       socket_type socket,
                       const boost::asio::const_buffer & source,
                       int flags) {
        msg.rebuild(boost::asio::buffer_size(source));
        boost::asio::buffer_copy(buffer(msg), source);
        return send(socket, &msg, flags);
    }

    template<typename ConstBufferSequence>
    expected_size send(message & msg,
                       socket_type socket,
                       const ConstBufferSequence &,
                       int flags,
                       typename ConstBufferSequence::const_iterator it,
                       dont_wait_t) {
        msg.rebuild(boost::asio::buffer_size(*it));
        boost::asio::buffer_copy(buffer(msg), *it);
        return send(socket, &msg, flags, dont_wait_t());
    }

    template<typename ConstBufferSequence>
    expected_size send(message & msg,
                       socket_type socket,
                       const ConstBufferSequence & buffers,
                       int flags) {
        size_t bytes_transferred = 0;
        for (auto it = std::begin(buffers), e = std::end(buffers); it != e; it++) {
            auto rc = send(msg, socket, *it, flags);
            if (!rc.valid())
                return rc;
            bytes_transferred += rc.get();
        }
        return bytes_transferred;
    }

    template<typename ConstBufferSequence>
    expected_size send(message & msg,
                       socket_type socket,
                       const ConstBufferSequence & buffers,
                       int flags,
                       send_more_t) {
        flags &= ~send_more_t::value;
        auto it = std::begin(buffers);
        auto e = std::end(buffers);
        if (it == e)
            return 0;

        size_t bytes_transferred = 0;
        while (std::distance(it, e) > 1) {
            auto rc = send(msg, socket, *it++, flags | send_more_t::value);
            if (!rc.valid())
                return rc;
            bytes_transferred += rc.get();
        }
        auto rc = send(msg, socket, *it, flags & ~send_more_t::value);
        if (!rc.valid())
            return rc;
        return bytes_transferred + rc.get();
    }

    inline
    expected_size receive(message & msg,
                          socket_type socket,
                          const boost::asio::mutable_buffer & target,
                          int flags) {
        msg.rebuild();
        auto rc = receive(socket, &msg, flags);
        if (!rc.valid()) return rc;
        if (boost::asio::buffer_size(target) < rc.get()) {
            auto ec = make_error_code(boost::system::errc::no_buffer_space);
            return expected_size::from_exception(boost::system::system_error(ec));
        }
        return boost::asio::buffer_copy(target, buffer(msg));
    }


    template<typename MutableBufferSequence>
    expected_size receive(message & msg,
                          socket_type socket,
                          const MutableBufferSequence & buffers,
                          int flags,
                          typename MutableBufferSequence::const_iterator & it,
                          dont_wait_t) {
        msg.rebuild();
        auto rc = receive(socket, &msg, flags, dont_wait_t());
        if (!rc.valid()) return rc;
        if (boost::asio::buffer_size(*it) < rc.get()) {
            auto ec = make_error_code(boost::system::errc::no_buffer_space);
            return expected_size::from_exception(boost::system::system_error(ec));
        }
        return boost::asio::buffer_copy(*it, buffer(msg));
    }

    template<typename MutableBufferSequence>
    expected_more receive(message & msg,
                          socket_type socket,
                          const MutableBufferSequence & buffers,
                          int flags,
                          receive_more_t) {
        size_t bytes_transferred = 0;
        try {
            flags &= ~receive_more_t::value;
            for (auto it = std::begin(buffers); it != std::end(buffers); ++it) {
                auto rc = receive(msg, socket, *it, flags);
                bytes_transferred += rc.get();
                if (!msg.more())
                    break;
            }
        } catch (...) {
            return expected_more::from_exception();
        }
        return std::make_pair(bytes_transferred, msg.more());
    }

    template<typename MutableBufferSequence>
    expected_size receive(message & msg,
                          socket_type socket,
                          const MutableBufferSequence & buffers,
                          int flags) {
        size_t bytes_transferred = 0;
        for (auto it = std::begin(buffers); it != std::end(buffers); ++it) {
            auto rc = receive(msg, socket, *it, flags);
            if (!rc.valid())
                return rc;
            bytes_transferred += rc.get();
        }
        return bytes_transferred;
    }

    inline
    boost::system::error_code monitor(socket_type socket,
                                      const endpoint_type & addr,
                                      int events,
                                      boost::system::error_code & ec) {
        BOOST_ASSERT_MSG(socket, "Invalid socket");

        auto rc = zmq_socket_monitor(socket, addr.c_str(), events);
        if (rc)
            ec = make_error_code();
        return ec;
    }
} // socket_ops
} // detail
} // aziomq
#endif // AZIOMQ_SOCKET_OPS_HPP_

