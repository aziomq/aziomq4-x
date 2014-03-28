/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_ZEROMQ_RECEIVE_OP_HPP_
#define AZIOMQ_ZEROMQ_RECEIVE_OP_HPP_

#include "tracked_op.hpp"
#include "zeromq_message.hpp"
#include "zeromq_socket_ops.hpp"

#include <boost/asio/detail/reactor_op.hpp>
#include <boost/asio/detail/bind_handler.hpp>
#include <boost/asio/detail/fenced_block.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>

#include <zmq.h>

#include <algorithm>

namespace aziomq { namespace detail {

template<typename MutableBufferSequence>
class zeromq_receive_op_base : public reactor_op
    AZIOMQ_ALSO_INERIT_TRACKED_OP {
    typedef typename MutableBufferSequence::const_iterator const_iterator;

public:
    zeromq_receive_op_base(socket_ops::socket_type socket,
                           const MutableBufferSequence & buffers,
                           int flags,
                           func_type complete_func) :
        reactor_op(socket, flags, select_func(buffers, flags), complete_func),
        buffers_(buffers),
        it_(std::begin(buffers)),
        end_(std::end(buffers)),
        more_(false) {
            AZIOMQ_TRACKED_OP_INIT(*this, "receive_op");
        }

    static bool do_perform_receive_more(boost::asio::detail::reactor_op* base) {
        auto o = static_cast<zeromq_receive_op_base*>(base);
        AZIOMQ_TRACKED_OP_LOG(*o, "do_perform(more)");

        o->ec_ = boost::system::error_code();
        try {
            auto rc = socket_ops::receive(o->msg_, o->socket_, o->buffers_,
                                            o->flags_, socket_ops::receive_more_t());
            auto mr = rc.get();
            o->bytes_transferred_ = mr.first;
            o->more_ = mr.second;
        } catch (const boost::system::system_error & e) {
            o->ec_ = e.code();
        }
        return true;
    }

    static bool do_perform(boost::asio::detail::reactor_op* base) {
        auto o = static_cast<zeromq_receive_op_base*>(base);
        AZIOMQ_TRACKED_OP_LOG(*o, "do_perform");

        o->ec_ = boost::system::error_code();
        try {
            auto rc = socket_ops::receive(o->msg_, o->socket_, o->buffers_,
                                            o->flags_, o->it_,
                                            socket_ops::dont_wait_t());
            auto bt = rc.get();
            if (bt != 0)
                ++o->it_;
            o->bytes_transferred_ += bt;
        } catch (const boost::system::system_error & e) {
            o->ec_ = e.code();
        }
        return o->it_ == o->end_;
    }

protected:
    bool more() const { return more_; }

private:
    static perform_func_type select_func(const MutableBufferSequence & buffers, int flags) {
        if (std::distance(std::begin(buffers), std::end(buffers)) == 0)
            return nullptr;

        return (flags & ZMQ_RCVMORE) ? &zeromq_receive_op_base::do_perform_receive_more
                                     : &zeromq_receive_op_base::do_perform;
    }

    const MutableBufferSequence & buffers_;
    const_iterator it_;
    const_iterator end_;
    bool more_;
};

template<typename MutableBufferSequence,
         typename Handler>
class zeromq_receive_op : public zeromq_receive_op_base<MutableBufferSequence> {
public:
    BOOST_ASIO_DEFINE_HANDLER_PTR(zeromq_receive_op);

    zeromq_receive_op(socket_ops::socket_type socket,
                      const MutableBufferSequence & buffers,
                      Handler handler,
                      int flags) :
        zeromq_receive_op_base<MutableBufferSequence>(socket, buffers, flags,
                &zeromq_receive_op::do_complete),
        handler_(std::move(handler)) { }

    static void do_complete(boost::asio::detail::io_service_impl* owner,
                            boost::asio::detail::operation* base,
                            const boost::system::error_code&,
                            size_t) {
        auto o = static_cast<zeromq_receive_op*>(base);
        AZIOMQ_TRACKED_OP_ON_COMPLETE(*o, o->ec_, o->bytes_transferred_);

        ptr p = { boost::asio::detail::addressof(o->handler_), o, o };

        BOOST_ASIO_HANDLER_COMPLETION((o));

        if (o->more())
            o->ec_ = make_error_code(boost::system::errc::no_buffer_space);

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
    Handler handler_;
};

template<typename MutableBufferSequence,
         typename Handler>
class zeromq_receive_more_op : public zeromq_receive_op_base<MutableBufferSequence> {
public:
    BOOST_ASIO_DEFINE_HANDLER_PTR(zeromq_receive_more_op);

    zeromq_receive_more_op(socket_ops::socket_type socket,
                      const MutableBufferSequence & buffers,
                      Handler handler,
                      int flags) :
        zeromq_receive_op_base<MutableBufferSequence>(socket, buffers, flags,
                &zeromq_receive_more_op::do_complete),
        handler_(std::move(handler)) { }

    static void do_complete(boost::asio::detail::io_service_impl* owner,
                            boost::asio::detail::operation* base,
                            const boost::system::error_code&,
                            size_t) {
        auto o = static_cast<zeromq_receive_more_op*>(base);
        AZIOMQ_TRACKED_OP_ON_COMPLETE(*o, o->ec_, o->bytes_transferred_);

        ptr p = { boost::asio::detail::addressof(o->handler_), o, o };

        BOOST_ASIO_HANDLER_COMPLETION((o));

        auto mr = std::make_pair(o->bytes_transferred_, o->more());
        boost::asio::detail::binder2<Handler, boost::system::error_code, socket_ops::more_result>
            handler(o->handler_, o->ec_, mr);
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
    Handler handler_;
};
} // detail
} // aziomq
#endif // AZIOMQ_ZEROMQ_RECEIVE_OP_HPP_

