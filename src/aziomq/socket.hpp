/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_IO_SOCKET_HPP_
#define AZIOMQ_IO_SOCKET_HPP_

#include "error.hpp"
#include "io_service.hpp"
#include "option.hpp"
#include "util/expected.hpp"
#include "detail/zeromq_message.hpp"
#include "detail/zeromq_socket_service.hpp"

#include <boost/optional.hpp>
#include <boost/asio/basic_io_object.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>
#include <boost/format.hpp>

namespace aziomq {
    /** \brief Implement an asio-like socket over a zeromq socket
     *  \remark sockets are movable, but not copyable
     *
     *  \remark Notes on thread safety (ZeroMQ interaction) -
     *      The [ZeroMQ Guide](http://zguide.zeromq.org/page:all#toc45) has extensive
     *      information on the ZeroMQ philosphy on how to write correct multi-threaded
     *      programs. This approach is certainly compatible with Asio, start an
     *      io_service per thread, and use the io_service to run an epoll reactor
     *      and use with inproc:// sockets to communicate between threads.  Asio
     *      however does allow multiple threads to participate in running an
     *      io_service. The Aziomq socket_service handles calls on behalf
     *      of a number of aziomq::socket's associated with a given Asio io_service.
     *      Asio's underlying epoll reactor guards dispatch to these operations with
     *      a mutex, which *should* meet the full fence requirement as noted in
     *      [ZeroMQ](http://api.zeromq.org/3-2:zmq-socket), but it not the ZeroMQ
     *      way, and *may* not be safe on all hardware architectures.
     */
    class socket :
        public boost::asio::basic_io_object<io_service::service_type> {

        friend detail::zeromq_socket_service::core_access;

    public:
        using native_handle_type = service_type::native_handle_type;
        using endpoint_type = boost::optional<std::string>;
        using message_flags = service_type::message_flags;
        using more_result = service_type::more_result;
        using proxy_type = service_type::proxy_type;

        // socket options
        using type = opt::type;
        using rcv_more = opt::rcv_more;
        using rcv_hwm = opt::rcv_hwm;
        using snd_hwm = opt::snd_hwm;
        using affinity = opt::affinity;
        using rate = opt::rate;
        using subscribe = opt::subscribe;
        using unsubscribe = opt::unsubscribe;
        using identity = opt::identity;
        using snd_buf = opt::snd_buf;
        using rcv_buf = opt::rcv_buf;
        using linger = opt::linger;
        using reconnect_ivl = opt::reconnect_ivl;
        using reconnect_ivl_max = opt::reconnect_ivl_max;
        using backlog = opt::backlog;
        using max_msgsize = opt::max_msgsize;
        using multicast_hops = opt::multicast_hops;
        using rcv_timeo = opt::rcv_timeo;
        using snd_timeo = opt::snd_timeo;
        using ipv6 = opt::ipv6;
        using ipv4_only = opt::ipv4_only;
        using immediate = opt::immediate;
        using router_mandatory = opt::router_mandatory;
        using router_raw = opt::router_raw;
        using probe_router = opt::probe_router;
        using xpub_verbose = opt::xpub_verbose;
        using req_correlate = opt::req_correlate;
        using req_relaxed = opt::req_relaxed;
        using tcp_keepalive = opt::tcp_keepalive;
        using tcp_keepalive_idle = opt::tcp_keepalive_idle;
        using tcp_keepalive_cnt = opt::tcp_keepalive_cnt;
        using tcp_keepalive_intvl = opt::tcp_keepalive_intvl;
        using tcp_accept_filter = opt::tcp_accept_filter;
        using plain_server = opt::plain_server;
        using plain_username = opt::plain_username;
        using plain_password = opt::plain_password;
        using curve_server = opt::curve_server;
        using curve_publickey = opt::curve_publickey;
        using curve_privatekey = opt::curve_privatekey;
        using zap_domain = opt::zap_domain;
        using conflate = opt::conflate;

        using shutdown_type = detail::socket_ops::shutdown_type;

        /** \brief socket constructor
         *  \param ios reference to an asio::io_service
         *  \param s_type int socket type
         *      For socket types see the zeromq documentation
         */
        explicit socket(boost::asio::io_service& ios, int type) :
            boost::asio::basic_io_object<io_service::service_type>(ios) {
            boost::system::error_code ec;
            get_service().construct(implementation);

            if (get_service().do_open(implementation, type, ec))
                throw boost::system::system_error(ec);
        }

        socket(socket && other) :
            boost::asio::basic_io_object<io_service::service_type>(other.get_io_service()) {
            get_service().move_construct(implementation, other.get_service(),
                                            other.implementation);
        }

        socket & operator=(socket && other) {
            get_service().move_assign(implementation, other.get_service(),
                                        other.implementation);
            return *this;
        }

        socket(const socket &) = delete;
        socket & operator=(const socket &) = delete;

        /** \brief Accept incoming connections on this socket
         *  \param addr const std::string& zeromq URI to bind
         *  \param ec error_code to capture error
         *  \see http://api.zeromq.org/4-1:zmq-bind
         */
        boost::system::error_code bind(const std::string & addr,
                                       boost::system::error_code & ec) {
            return get_service().bind(implementation, addr, ec);
        }

        /** \brief Accept incoming connections on this socket
         *  \param addr const std::string& zeromq URI to bind
         *  \throw boost::system::system_error
         *  \see http://api.zeromq.org/4-1:zmq-bind
         */
        void bind(const std::string & addr) {
            boost::system::error_code ec;
            if (bind(addr, ec))
                throw boost::system::system_error(ec);
        }

        /** \brief Create outgoing connection from this socket
         *  \param addr const std::string& zeromq URI of endpoint
         *  \param ec error_code to capture error
         *  \see http://api.zeromq.org/4-1:zmq-connect
         */
        boost::system::error_code connect(const std::string & addr,
                                          boost::system::error_code & ec) {
            return get_service().connect(implementation, addr, ec);
        }

        /** \brief Create outgoing connection from this socket
         *  \param addr const std::string& zeromq URI of endpoint
         *  \throw boost::system::system_error
         *  \see http://api.zeromq.org/4-1:zmq-connect
         */
        void connect(const std::string & addr) {
            boost::system::error_code ec;
            if (connect(addr, ec))
                throw boost::system::system_error(ec);
        }

        /** \brief return endpoint addr supplied to bind or connect
         *  \returns boost::optional<std::string>
         *  \remarks Return value will be empty if bind or connect has
         *  not yet been called/succeeded.  If multiple calls to connect
         *  or bind have occured, this call wil return only the first
         */
        endpoint_type endpoint() const {
            auto res = get_service().endpoint(implementation);
            if (!res.empty())
                return res;
            return endpoint_type();
        }

        /** \brief Set an option on a socket
         *  \tparam Option type which must conform the asio SettableSocketOption concept
         *  \param ec error_code to capture error
         *  \param opt T option to set
         */
        template<typename Option>
        boost::system::error_code set_option(const Option & opt,
                                             boost::system::error_code & ec) {
            return get_service().set_option(implementation, opt, ec);
        }

        /** \brief Set an option on a socket
         *  \tparam T type which must conform the asio SettableSocketOption concept
         *  \param opt T option to set
         *  \throw boost::system::system_error
         */
        template<typename Option>
        void set_option(const Option & opt) {
            boost::system::error_code ec;
            if (set_option(opt, ec))
                throw boost::system::system_error(ec);
        }

        /** \brief Get an option from a socket
         *  \tparam T must conform to the asio GettableSocketOption concept
         *  \param opt T option to get
         *  \param ec error_code to capture error
         */
        template<typename Option>
        boost::system::error_code get_option(Option & opt,
                                             boost::system::error_code & ec) {
            return get_service().get_option(implementation, opt, ec);
        }

        /** \brief Get an option from a socket
         *  \tparam T must conform to the asio GettableSocketOption concept
         *  \param opt T option to get
         *  \throw boost::system::system_error
         */
        template<typename Option>
        void get_option(Option & opt) {
            boost::system::error_code ec;
            if (get_option(opt, ec))
                throw boost::system::system_error(ec);
        }

        /** \brief Receive some data from the socket
         *  \tparam MutableBufferSequence
         *  \param buffers buffer(s) to fill on receive
         *  \param flags specifying how the receive call is to be made
         *  \param ec set to indicate what error, if any, occurred
         *  \remark
         *  If buffers is a sequence of buffers, and flags has ZMQ_RCVMORE
         *  set, this call will fill the supplied sequence with message
         *  parts from a multipart message. It is possible that there are
         *  more message parts than supplied buffers, or that an individual
         *  message part's size may exceed an individual buffer in the
         *  sequence. In either case, the call will return with ec set to
         *  no_buffer_space. It is the callers responsibility to issue
         *  additional receive calls to collect the remaining message parts.
         *
         * \remark
         * If flags does not have ZMQ_RCVMORE set, this call will synchronously
         * receive a message for each buffer in the supplied sequence
         * before returning.
         */
        template<typename MutableBufferSequence>
        std::size_t receive(const MutableBufferSequence & buffers,
                            message_flags flags,
                            boost::system::error_code & ec) {
            return get_service().receive(implementation, buffers, flags, ec);
        }

        /** \brief Receive some data from the socket
         *  \tparam MutableBufferSequence
         *  \param buffers buffer(s) to fill on receive
         *  \param flags flags specifying how the receive call is to be made
         *  \throw boost::system::system_error
         *  \remark
         *  If buffers is a sequence of buffers, and flags has ZMQ_RCVMORE
         *  set, this call will fill the supplied sequence with message
         *  parts from a multipart message. It is possible that there are
         *  more message parts than supplied buffers, or that an individual
         *  message part's size may exceed an individual buffer in the
         *  sequence. In either case, the call will return with ec set to
         *  no_buffer_space. It is the callers responsibility to issue
         *  additional receive calls to collect the remaining message parts.
         *
         * \remark
         * If flags does not have ZMQ_RCVMORE set, this call will synchronously
         * receive a message for each buffer in the supplied sequence
         * before returning.
         */
        template<typename MutableBufferSequence>
        std::size_t receive(const MutableBufferSequence & buffers,
                            message_flags flags = 0) {
            boost::system::error_code ec;
            auto res = receive(buffers, flags, ec);
            if (ec)
                throw boost::system::system_error(ec);
            return res;
        }

        /** \brief Receive some data as part of a multipart message from the socket
         *  \tparam MutableBufferSequence
         *  \param buffers buffer(s) to fill on receive
         *  \param flags specifying how the receive call is to be made
         *  \param ec set to indicate what error, if any, occurred
         *  \return pair<size_t, bool>
         *  \remark
         *  Works as for receive() with flags containing ZMQ_RCV_MORE but returns
         *  a pair containing the number of bytes transferred and a boolean flag
         *  which if true, indicates more message parts are available on the
         *  socket.
         */
        template<typename MutableBufferSequence>
        more_result receive_more(const MutableBufferSequence & buffers,
                                 message_flags flags,
                                 boost::system::error_code & ec) {
            return get_service().receive_more(implementation, buffers, flags, ec);
        }

        /** \brief Receive some data as part of a multipart message from the socket
         *  \tparam MutableBufferSequence
         *  \param buffers buffer(s) to fill on receive
         *  \param flags specifying how the receive call is to be made
         *  \return pair<size_t, bool>
         *  \throw boost::system::system_error
         *  \remark
         *  Works as for receive() with flags containing ZMQ_RCV_MORE but returns
         *  a pair containing the number of bytes transferred and a boolean flag
         *  which if true, indicates more message parts are available on the
         *  socket.
         */
        template<typename MutableBufferSequence>
        more_result receive_more(const MutableBufferSequence & buffers,
                                 message_flags flags = 0) {
            boost::system::error_code ec;
            auto res = receive_more(buffers, flags, ec);
            if (ec)
                throw boost::system::system_error(ec);
            return res;
        }

        /** \brief Send some data from the socket
         *  \tparam ConstBufferSequence
         *  \param buffers buffer(s) to send
         *  \param flags specifying how the send call is to be made
         *  \param ec set to indicate what, if any, error occurred
         *  \remark
         *  If buffers is a sequence of buffers, and flags has ZMQ_SNDMORE
         *  set, this call will construct a multipart message from the supplied
         *  buffer sequence.
         *
         * \remark
         * If flags does not have ZMQ_RCVMORE set, this call will synchronously
         * send a message for each buffer in the supplied sequence before
         * returning.
         */
        template<typename ConstBufferSequence>
        std::size_t send(const ConstBufferSequence & buffers,
                         message_flags flags,
                         boost::system::error_code & ec) {
            return get_service().send(implementation, buffers, flags, ec);
        }

        /** \brief Send some data to the socket
         *  \tparam ConstBufferSequence
         *  \param buffers buffer(s) to send
         *  \param flags specifying how the send call is to be made
         *  \throw boost::system::system_error
         *  \remark
         *  If buffers is a sequence of buffers, and flags has ZMQ_SNDMORE
         *  set, this call will construct a multipart message from the supplied
         *  buffer sequence.
         *
         * \remark
         * If flags does not have ZMQ_RCVMORE set, this call will synchronously
         * send a message for each buffer in the supplied sequence before
         * returning.
         */
        template<typename ConstBufferSequence>
        std::size_t send(const ConstBufferSequence & buffers,
                         message_flags flags = 0) {
            boost::system::error_code ec;
            auto res = send(buffers, flags, ec);
            if (ec)
                throw boost::system::system_error(ec);
            return res;
        }

        /** \brief Initiate an async receive operation.
         *  \tparam MutableBufferSequence
         *  \tparam ReadHandler must conform to the asio ReadHandler concept
         *  \param buffers buffer(s) to fill on receive
         *  \param handler ReadHandler
         *  \remark
         *  If buffers is a sequence of buffers, and flags has ZMQ_RCVMORE
         *  set, this call will fill the supplied sequence with message
         *  parts from a multipart message. It is possible that there are
         *  more message parts than supplied buffers, or that an individual
         *  message part's size may exceed an individual buffer in the
         *  sequence. In either case, the handler will be called with ec set
         *  to no_buffer_space. It is the callers responsibility to issue
         *  additional receive calls to collect the remaining message parts.
         *  If any message parts remain after the call to the completion
         *  handler returns, the socket handler will throw an exception to
         *  the io_service forcing this socket to be removed from the poll
         *  set. The socket is largely unusable after this, in particular
         *  any subsequent call to (async_)send/receive will raise an exception.
         *
         * \remark
         *  If flags does not have ZMQ_RCVMORE set, this call will asynchronously
         *  receive a message for each buffer in the supplied sequence before
         *  calling the supplied handler.
         */
        template<typename MutableBufferSequence,
                 typename ReadHandler>
        void async_receive(const MutableBufferSequence & buffers,
                           ReadHandler handler,
                           message_flags flags = 0) {
            get_service().async_receive(implementation, buffers, std::move(handler), flags);
        }

        /** \brief Initiate an async receive operation.
         *  \tparam MutableBufferSequence
         *  \tparam ReadMoreHandler must conform to the ReadMoreHandler concept
         *  \param buffers buffer(s) to fill on receive
         *  \param handler ReadMoreHandler
         *  \remark
         *  The ReadMoreHandler concept has the following interface
         *      struct ReadMoreHandler {
         *          void operator()(const boost::system::error_code & ec,
         *                          more_result result);
         *      }
         *  \remark
         *  Works as for async_receive() with flags containing ZMQ_RCV_MORE but
         *  does not error if more parts remain than buffers supplied.  The
         *  completion handler will be called with a more_result indicating the
         *  number of bytes transferred thus far, and flag indicating whether
         *  more message parts remain. The handler may then make syncrhonous
         *  receive_more() calls to collect the remaining message parts.
         */
        template<typename MutableBufferSequence,
                 typename ReadMoreHandler>
        void async_receive_more(const MutableBufferSequence & buffers,
                                ReadMoreHandler handler,
                                message_flags flags = 0) {
            get_service().async_receive_more(implementation, buffers,
                                             std::move(handler), flags);
        }

        /** \brief Initiate an async send operation
         *  \tparam ConstBufferSequence must conform to the asio
         *          ConstBufferSequence concept
         *  \tparam WriteHandler must conform to the asio
         *          WriteHandler concept
         *  \param flags specifying how the send call is to be made
         *  \remark
         *  If buffers is a sequence of buffers, and flags has ZMQ_SNDMORE
         *  set, this call will construct a multipart message from the supplied
         *  buffer sequence.
         *
         *  \remark
         *  If flags does not specify ZMQ_SNDMORE this call will asynchronously
         *  send each buffer in the sequence as an individual message.
         */
        template<typename ConstBufferSequence,
                 typename WriteHandler>
        void async_send(const ConstBufferSequence& buffers,
                        WriteHandler handler,
                        message_flags flags = 0) {
            get_service().async_send(implementation, buffers,
                                        std::move(handler), flags);
        }

        boost::system::error_code shutdown(shutdown_type what,
                                           boost::system::error_code & ec) {
            get_service().shutdown(implementation, what, ec);
            return ec;
        }

        void shutdown(shutdown_type what) {
            boost::system::error_code ec;
            if (shutdown(what, ec))
                throw boost::system::system_error(ec);
        }

        /** \brief Get a pointer to the zmq socket
         */
        native_handle_type native_handle() const {
            return get_service().native_handle(implementation);
        }

        /** \brief monitor events on this socket
         *  \tparam Handler handler function which conforms to the SocketMonitorHandler concept
         *  \param ios io_service on which to bind the returned monitor socket
         *  \param handler Handler conforming to the SocketMonitorHandler concept as follows
         *     void SocketMonitorHandler(boost::system::error_code const& ec,
         *                               zmq_event_t const& event,
         *                               boost::asio::const_buffer const& addr);
         *  \param events int mask of events to publish to returned socket
         *  \param ec error_code to set on error
         *  \returns socket
         *  \remark
         *  The returned socket will be constructed on the supplied ios as a ZMQ_PAIR socket
         *  type bound on the inproc transport to this socket's monitor. The supplied handler will
         *  be registered to receive events asynchronously on the returned socket as long
         *  as the returned socket remains in scope.
        **/
        template<typename Handler>
        socket monitor(boost::asio::io_service & ios,
                       Handler handler,
                       int events,
                       boost::system::error_code & ec) {
            socket res(ios, ZMQ_PAIR);
            auto a = monitor_addr();
            if (get_service().monitor(implementation, a, events, ec))
                return res;

            if (res.connect(a, ec))
                return res;

            res.associate_handler(std::move(handler), a);
            return res;
        }

        /** \brief monitor events on this socket
         *  \tparam Handler handler function which conforms to the SocketMonitorHandler concept
         *  \param ios io_service on which to bind the returned monitor socket
         *  \param handler Handler conforming to the SocketMonitorHandler concept as follows
         *     void SocketMonitorHandler(boost::system::error_code const& ec,
         *                               zmq_event_t const& event,
         *                               boost::asio::const_buffer const& addr);
         *  \param events int mask of events to publish to returned socket
         *  \returns socket
         *  \throws boost::system::system_error
         *  \remark
         *  The returned socket will be constructed on the supplied ios as a ZMQ_PAIR socket
         *  type bound on the inproc transport to this socket's monitor. The supplied handler will
         *  be registered to receive events asynchronously on the returned socket as long
         *  as the returned socket remains in scope.
        **/
        template<typename Handler>
        socket monitor(boost::asio::io_service & ios,
                       Handler handler,
                       int events = ZMQ_EVENT_ALL) {
            boost::system::error_code ec;
            auto res = monitor(ios, std::move(handler), events, ec);
            if (ec)
                throw boost::system::system_error(ec);
            return res;
        }

        /** \brief Create a proxy connection between two sockets
         *  \param frontend socket
         *  \param backened socket
         *  \return shared_ptr of an implementation defined proxy handle
         *  \remark
         *  Works like zmq_proxy API but is non blocking, the proxy runs
         *  as long as the returned proxy handle is not destroyed.
         */
        static proxy_type proxy(socket & frontend, socket & backend) {
            BOOST_ASSERT_MSG(&frontend.get_service() == &backend.get_service(),
                    "Sockets must belong to the same io_service");
            return frontend.get_service().register_proxy(frontend.implementation,
                                                         backend.implementation);
        }

        /** \brief Create a proxy connection between two sockets
         *  \param frontend socket
         *  \param backened socket
         *  \param capture socket
         *  \return shared_ptr of an implementation defined proxy handle
         *  \remark
         *  Works like zmq_proxy API but is non blocking, the proxy runs
         *  as long as the returned proxy handle is not destroyed.
         */
        static proxy_type proxy(socket & frontend, socket & backend, socket & capture) {
            BOOST_ASSERT_MSG(&frontend.get_service() == &backend.get_service(),
                    "Sockets must belong to the same io_service");
            return frontend.get_service().register_proxy(frontend.implementation,
                                                         backend.implementation,
                                                         capture.implementation);
        }

        /** \brief Allows the caller to associate an instance which conforms to the
         *  following protocol:
         *      struct handler {
         *          void on_install();
         *
         *          template<typename Option>
         *          error_code set_option(Option const&, error_code &);
         *
         *          template<typename Option>
         *          error_code get_option(Option &, error_code &);
         *      };
         *  with the supplied socket.
         *  \remark Only one such registration may be made
         *  per socket.  The supplied type will have it's on_install() method called
         *  when the instance is installing on the socket.  The type will have
         *  the same lifetime as the owning socket.  If on_install() throws, the
         *  exception will propagate through this call and the socket will not be
         *  changed.
         *  \remark set/get_option allows the caller to interract with the handler
         *  from the socket interface. If the handler does not support the supplied,
         *  option, the handler should return errc::not_supported.  Handler options
         *  should satisfy the following protocol:
         *      struct option {
         *          int name() const;
         *          const void* data() const;
         *          void* data();
         *          size_t size() const;
         *          void resize(size_t);
         *      };
         *  \returns true if the handler was installed, false if a handler is already
         *  associated.
         */
        template<typename T>
        bool associate_handler(T handler) {
            handler_model<T> h(std::move(handler));
            return get_service().associate_handler<handler_model<T>>(implementation, std::move(h));
        }

    private:
        template<typename T>
        struct handler_model
            : detail::zeromq_socket_service::assoc_handler
        {
            handler_model(T d) : data_(std::move(d)) { }
            handler_model(handler_model && rhs) : data_(std::move(rhs.data_)) { }

            handler_model(const handler_model&) = delete;
            handler_model & operator=(const handler_model &) = delete;

            void on_install() override { data_.on_install(); }
            boost::system::error_code set_option(service_type::opt_concept const& opt,
                                                 boost::system::error_code & ec) override {
                return data_.set_option(opt, ec);
            }

            boost::system::error_code get_option(service_type::opt_concept & opt,
                                                 boost::system::error_code & ec) override {
                return data_.get_option(opt, ec);
            }

            T data_;
        };

        static std::string monitor_addr() {
            static std::atomic<unsigned long> id(0);
            return boost::str(boost::format("inproc://monitor.%1%") % ++id);
        }

        struct monitor_impl
            : detail::zeromq_socket_service::assoc_handler
            , std::enable_shared_from_this<monitor_impl>
        {
            socket & self_;
            std::function<void(boost::system::error_code const&,
                               zmq_event_t const&,
                               boost::asio::const_buffer const&)> func_;
            std::string monitor_addr_;
            zmq_event_t event_;
            std::array<char, 1025> addr_;
            std::array<boost::asio::mutable_buffer, 2> bufs_;

            template<typename Handler>
            monitor_impl(socket & self, Handler handler, const std::string & monitor_addr)
                : self_(self)
                , func_(std::move(handler))
                , monitor_addr_(monitor_addr)
                , bufs_{ boost::asio::buffer(&event_, sizeof(zmq_event_t)),
                         boost::asio::buffer(addr_) }
            { }

            void on_event(boost::system::error_code const& ec, size_t) {
                func_(ec, event_, boost::asio::buffer(addr_));
                addr_.fill(0);
            }

            void run(std::weak_ptr<monitor_impl> p) {
                self_.async_receive(bufs_,
                                    [p](const boost::system::error_code & ec, size_t bytes_transferred) {
                    if (auto pp = p.lock()) {
                        pp->on_event(ec, bytes_transferred);
                        pp->run(p);
                    }
                });
            }

            virtual void on_install() { run(shared_from_this()); }
            boost::system::error_code set_option(service_type::opt_concept const&,
                                                 boost::system::error_code & ec) override {
                return ec = make_error_code(boost::system::errc::not_supported);
            }

            boost::system::error_code get_option(service_type::opt_concept &,
                                                 boost::system::error_code & ec) override {
                return ec = make_error_code(boost::system::errc::not_supported);
            }
        };

        template<typename Handler>
        void associate_handler(Handler handler, const std::string & monitor_addr) {
            get_service().associate_handler<monitor_impl>(implementation, *this, std::move(handler), monitor_addr);
        }
    };
}
#endif // AZIOMQ_IO_SOCKET_HPP_
