/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_IO_SERVICE_HPP_
#define AZIOMQ_IO_SERVICE_HPP_

#include "detail/zeromq_socket_service.hpp"
#include "option.hpp"
#include "error.hpp"

#include <boost/asio/io_service.hpp>
#include <zmq.h>

#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <functional>

namespace aziomq {
namespace io_service {
    using  service_type = detail::zeromq_socket_service;

    using io_threads = opt::io_threads;
    using max_sockets = opt::max_sockets;
    using ipv6 = opt::ipv6;

    /** \brief set options on the zeromq context.
     *  \tparam Option option type
     *  \param option Option option to set
     *  \remark Must be called before any sockets are created
     */
    template<typename Option>
    boost::system::error_code set_option(boost::asio::io_service & io_service,
                                         const Option & option,
                                         boost::system::error_code & ec) {

        return boost::asio::use_service<service_type>(io_service).set_option(option, ec);
    }

    /** \brief set options on the zeromq context.
     *  \tparam Option option type
     *  \param option Option option to set
     *  \remark Must be called before any sockets are created
     */
    template<typename Option>
    void set_option(boost::asio::io_service & io_service, const Option & option) {
        boost::system::error_code ec;
        if (set_option(io_service, option, ec))
            throw boost::system::system_error(ec);
    }

    /** \brief get option from zeromq context
     *  \tparam Option option type
     *  \param option Option te get
     *  \param ec boost::system::error_code
     */
    template<typename Option>
    boost::system::error_code get_option(boost::asio::io_service & io_service,
                                         Option & option,
                                         boost::system::error_code & ec) {
        return boost::asio::use_service<service_type>(io_service).get_option(option, ec);
    }

    /** \brief get option from zeromq context
     *  \tparam Option option type
     *  \param option Option te get
     *  \param ec boost::system::error_code
     */
    template<typename Option>
    void get_option(boost::asio::io_service & io_service, Option & option) {
        boost::system::error_code ec;
        if (get_option(io_service, option))
            throw boost::system::system_error(ec);
    }
} // io_service
} // aziomq
#if 0
    /** \brief provide ZeroMQ polling services to asio
     *  \remarks The first use of any aziomq type that takes a
     *  boost::asio::io_service will result in a single instance
     *  of this type being constructed and registered on the
     *  io_service.  By default this will construct the ZeroMQ
     *  context with one io thread.  If you wish to alter the
     *  number of io threads, you must call the register_service()
     *  static method on this type BEFORE any other calls.
     * */
    class io_service : public boost::asio::io_service::service {
    public:
        using pollitem_t = zmq_pollitem_t;
        static boost::asio::io_service::id id;

        /** \brief construct an aziomq io_service
         *  \param owner boost::asio::io_service& which owns this instance
         *  \param ctx void* previously constructed ZeroMQ context
         *  \remarks You should generally have no need to directly
         *  instatiate this type.
         */
        explicit io_service(boost::asio::io_service & owner, void * ctx) :
            boost::asio::io_service::service{ owner },
            strand_{ owner },
            ctx_{ ctx },
            next_{ 1 },
            pimpl_{ std::make_shared<impl>() }
            {
                if (!ctx_)
                    throw boost::system::system_error(make_error_code());
            }

        /** \brief construct an aziomq io_service
         *  \param owner boost::asio::io_service& which owns this instance
         *  \param int ZeroMQ io_threads to create
         *  \remarks You should generally have no need to directly
         *  instatiate this type.
         */
        explicit io_service(boost::asio::io_service & owner, int io_threads = 1) :
            io_service{ owner, zmq_init(io_threads) } { }

        /** \brief return a reference to the zmq::context used by this */
        void* context() { return ctx_; }

        class work_item_t;
        using work_item = std::shared_ptr<work_item_t>;

        /** \brief register a poll_item with this service
         *  \tparam F supplied fn must be a type implementing:
         *     bool operator()(short)
         *  \param item zmq::pollitem_t to register
         *  \param fn a callback function of type F
         *  \remarks Only ZeroMQ socket pollitem_t's may be registered
         *
         *  \remarks When fn is called, the short param supplies the revents
         *  field from the supplied poll item.  The callback should return
         *  true to remain active on the io_service, false if not.  Caller
         *  may later call reactivate, on a work item which is no longer active.
         *  If fn() throws, the exception will be captured and the item will be
         *  deactivated.
         *
         *  \remarks The returned item is held by a weak_ptr and will be removed
         *  when no external references to the work item remain
         */
        template<typename F>
        work_item register_item(pollitem_t item, F fn) {
            if (item.socket == 0)
                throw std::invalid_argument("item must have a valid zeromq socket");
            if (item.fd != 0)
                throw std::invalid_argument("item must be a zeromq socket, fd polling not supported");

            item.fd = next_++; // we use fd as a key

            auto res = std::make_shared<work_item_t>(item, std::move(fn), [this] {
                            notify_changed(strand_, pimpl_);
                       });

            register_item(strand_, pimpl_, res);
            if (started_.fetch_add(1) == 0)
                run(strand_, pimpl_);
            return res;
        }

        /** \brief Reactive a previously deactivated work_item
         *  \param item work_item previously returned from register_item
         */
        void reactivate(work_item item) { register_item(strand_, pimpl_, item); }


    private:
        boost::asio::strand strand_;
        void* ctx_;
        std::atomic<int> next_;
        std::atomic<long> started_;

        // things who's lifetime needs to be shared with background thread handlers
        struct impl {
            using weak_work_item_ptr = std::weak_ptr<work_item_t>;
            std::unordered_map<int, weak_work_item_ptr> work_;
            std::vector<int> dead_;
            std::vector<pollitem_t> poll_;

            void on_register_item(work_item ptr);
            void on_changed();
            void on_run(int key, short revents);
            void on_poll(boost::asio::strand & strand);
        };
        std::shared_ptr<impl> pimpl_;

        virtual void shutdown_service();

        static void register_item(boost::asio::strand & owner,
                                  std::weak_ptr<impl> pimpl,
                                  work_item item);

        static void notify_changed(boost::asio::strand & owner,
                                   std::weak_ptr<impl> pimpl);

        static void run(boost::asio::strand & owner,
                        std::weak_ptr<impl> pimpl);
    };

    class io_service::work_item_t :
        public std::enable_shared_from_this<io_service::work_item_t> {

    public:
        template<typename F, typename C>
        work_item_t(pollitem_t pollitem, F fn, C changefn) :
            ready_( true ),
            pollitem_(pollitem),
            fn_(std::move(fn)),
            changefn_(std::move(changefn)) { }

        ~work_item_t() {
            changefn_();
        }

        void reset() {
            ready_ = true;
            lasterror_ = std::exception_ptr();
        }

        int key() const { return pollitem_.fd; }

        bool run(short revents);

        const pollitem_t& item() const { return pollitem_; }
        pollitem_t& item() { return pollitem_; }

        void on_changed() { changefn_(); }

        bool ready() const { return has_error() ? false : ready_; }

        bool has_error() const {
            return lasterror_ != std::exception_ptr();
        }

        std::exception_ptr lasterror() {
            auto res = lasterror_;
            lasterror_ = std::exception_ptr();
            return res;
        }

        void check_lasterror() {
            auto err = lasterror();
            if (err != std::exception_ptr())
                std::rethrow_exception(err);
        }

    private:
        bool ready_;
        pollitem_t pollitem_;
        std::function<bool(short)> fn_;
        std::function<void(void)> changefn_;
        std::exception_ptr lasterror_;
    };
}
#endif
#endif // AZIOMQ_IO_SERVICE_HPP_
