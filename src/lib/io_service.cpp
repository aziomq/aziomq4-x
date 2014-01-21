/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#include "../aziomq/detail/tracked_op.hpp"
#include "../aziomq/detail/zeromq_socket_service.hpp"
#include "../aziomq/io_service.hpp"

#include <boost/optional.hpp>

#include <functional>
#include <string>
#include <ostream>

using namespace aziomq::detail;

boost::asio::io_service::id zeromq_socket_service::id;

zeromq_socket_service::mutex_type mtx;
std::weak_ptr<void> ctx;

zeromq_socket_service::context_pointer_type zeromq_socket_service::get_context() {
    std::unique_lock<std::mutex> lock(mtx);
    auto p = ctx.lock();
    if (!p) {
        ctx = p = zeromq_socket_service::context_pointer_type(
                                zmq_ctx_new(),
                                [](void *p) { zmq_term(p); });
    }
    return p;
}

zeromq_socket_service::mutex_type & zeromq_socket_service::static_mutex() {
    return mtx;
}

using ostream_ref = std::reference_wrapper<std::ostream>;
boost::optional<ostream_ref> log_stream;

void tracked_op::write_log(const std::string & what) {
    if (log_stream) {
        using lock_type = std::unique_lock<zeromq_socket_service::mutex_type>;
        auto l = lock_type(zeromq_socket_service::static_mutex());
        auto& out = log_stream.get().get();
        out << what << std::endl;
    }
}

void tracked_op::init(std::ostream & stm) {
    if (log_stream) return; // already set
    log_stream = std::ref(stm);
}


