/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_ZEROMQ_PROXY_OP_HPP_
#define AZIOMQ_ZEROMQ_PROXY_OP_HPP_

#include "tracked_op.hpp"
#include "zeromq_message.hpp"
#include "zeromq_socket_ops.hpp"

#include <boost/asio/detail/reactor_op.hpp>
#include <boost/asio/detail/bind_handler.hpp>
#include <boost/asio/detail/fenced_block.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>

#include <zmq.h>

#include <mutex>

namespace aziomq { namespace detail {
template<typename Handler>
class zeromq_proxy_op : public reactor_op
    AZIOMQ_ALSO_INERIT_TRACKED_OP {
public:
    BOOST_ASIO_DEFINE_HANDLER_PTR(zeromq_proxy_op);
    zeromq_proxy_op(socket_ops::socket_type frontend,
                    socket_ops::socket_type backend,
                    socket_ops::socket_type capture,
                    std::mutex & mtx,
                    Handler handler) :
        reactor_op(frontend, 0, &zeromq_proxy_op::do_perform,
                                &zeromq_proxy_op::do_complete),
        backend_(backend),
        capture_(capture),
        mtx_(mtx),
        handler_(std::move(handler)) {
            AZIOMQ_TRACKED_OP_INIT(*this, "proxy_op");
        }

    static bool do_perform(boost::asio::detail::reactor_op* base) {
        auto o = static_cast<zeromq_proxy_op*>(base);

        // prevent simultaneous access to underlying sockets here in case
        // io_service has more than one thread calling run()
        std::unique_lock<std::mutex> lock(o->mtx_);
        try {
            for (;;) {
                o->bytes_transferred_ += socket_ops::receive(o->socket_, &o->msg_, 0).get();
                if (o->capture_) {
                    message ctrl(o->msg_);
                    socket_ops::send(o->capture_, &ctrl, 0).get();
                }
                bool more = o->msg_.more();
                socket_ops::send(o->backend_, &o->msg_, more ? ZMQ_SNDMORE : 0).get();
                if (!more)
                    break;
            }
        } catch (const boost::system::system_error & e) {
            o->ec_ = e.code();
        }
        return true;
    }

    static void do_complete(boost::asio::detail::io_service_impl* owner,
                            boost::asio::detail::operation* base,
                            const boost::system::error_code&,
                            size_t) {
        auto o = static_cast<zeromq_proxy_op*>(base);
        AZIOMQ_TRACKED_OP_ON_COMPLETE(*o, o->ec_, o->bytes_transferred_);

        ptr p = { boost::asio::detail::addressof(o->handler_), o, o };

        BOOST_ASIO_HANDLER_COMPLETION((o));

        boost::asio::detail::binder2<Handler, boost::system::error_code, size_t>
            handler(o->handler_, o->ec_, o->bytes_transferred_);
        p.h = boost::asio::detail::addressof(handler.handler_);
        p.reset();

        if (owner) {
            boost::asio::detail::fenced_block b(boost::asio::detail::fenced_block::half);
            BOOST_ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_, handler.arg2_));
            boost_asio_handler_invoke_helpers::invoke(handler, handler.handler_);
            BOOST_ASIO_HANDLER_INVOCATION_END;
        }
    }

private:
    socket_ops::socket_type backend_;
    socket_ops::socket_type capture_;
    std::mutex & mtx_;
    Handler handler_;
};

} // detail
} // aziomq

#endif // AZIOMQ_ZEROMQ_PROXY_OP_HPP_
