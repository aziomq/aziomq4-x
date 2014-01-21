/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_ZEROMQ_TRACKED_OP_HPP_
#define AZIOMQ_ZEROMQ_TRACKED_OP_HPP_

#include <boost/system/error_code.hpp>

#include <ostream>
#include <sstream>
#include <string>

namespace aziomq {
namespace detail {

struct tracked_op {
    ~tracked_op() {
        if (op_name_.empty()) return;
        log("terminate");
    }

    void init(const std::string & name, bool is_noop) {
        op_name_ = name;
        tracked_stream s;
        s << op_name_ << " - init";
        if (is_noop)
            s << " (noop)";
    }

    void log(const std::string & what) {
        tracked_stream s;
        s << op_name_ << " - " << what;
    }

    void log_completion(const boost::system::error_code & ec,
                        size_t bytes_transferred) {
        tracked_stream s;
        s << op_name_ << " - on_complete (ec="
            << ec << ", bytes_transferred=" << bytes_transferred 
          << ")";
    }
    static void write_log(const std::string & what);
    static void init(std::ostream & stm);

private:
    struct tracked_stream : std::ostringstream {
        ~tracked_stream() {
            tracked_op::write_log(str());
        }
    };


    std::string op_name_;
};
} // namespace detail
} // namespace aziomq

#ifdef AZIOMQ_ENABLE_HANDLER_TRACKING
#define AZIOMQ_ALSO_INERIT_TRACKED_OP \
    , public tracked_op

#define AZIOMQ_TRACKED_OP_INIT(op, x) \
    (op).init((x), (op).is_noop())

#define AZIOMQ_TRACKED_OP_LOG(op, x) \
    do { \
        std::ostringstream __s; \
        __s << x; \
        (op).log(__s.str()); \
    } while(0)

#define AZIOMQ_TRACKED_OP_ON_COMPLETE(op, ec, bytes_transferred) \
    (op).log_completion((ec),(bytes_transferred))

#define AZIOMQ_TRACKED_LOG(x) \
    do { \
        std::ostringstream __s; \
        __s << x; \
        aziomq::detail::tracked_op::write_log(__s.str()); \
    } while(0)

#else
#define AZIOMQ_ALSO_INERIT_TRACKED_OP
#define AZIOMQ_TRACKED_OP_LOG(op, x) (void)0
#define AZIOMQ_TRACKED_OP_INIT(op, x) (void)0
#define AZIOMQ_TRACKED_OP_ON_COMPLETE(op, ec, bytes_transferred) (void)0
#define AZIOMQ_TRACKED_LOG(x) (void)0
#endif // AZIOMQ_ENABLE_HANDLER_TRACKING
#endif // AZIOMQ_ZEROMQ_TRACKED_OP_HPP_
