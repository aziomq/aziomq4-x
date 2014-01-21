/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_REACTOR_OP_HPP_
#define AZIOMQ_REACTOR_OP_HPP_

#include "zeromq_message.hpp"

#include <boost/asio/detail/reactor_op.hpp>

namespace aziomq {
namespace detail {
struct reactor_op : public boost::asio::detail::reactor_op {
    using socket_type = void *;

    bool is_noop() const { return socket_ == nullptr; }
    static bool is_noop(boost::asio::detail::reactor_op * base) {
        auto o = static_cast<reactor_op*>(base);
        return o->is_noop();
    }

protected:
    reactor_op(socket_type socket,
               int flags,
               perform_func_type perform_func, func_type complete_func) :
        boost::asio::detail::reactor_op(perform_func, complete_func),
        socket_(perform_func != nullptr ? socket : nullptr),
        flags_(flags),
        msg_(false) { }

    socket_type socket_;
    int flags_;
    message msg_;
};

} // detail
} // aziomq
#endif // AZIOMQ_REACTOR_OP_HPP_

