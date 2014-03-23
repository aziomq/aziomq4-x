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
#include "detail/expected.hpp"
#include "detail/zeromq_message.hpp"
#include "detail/zeromq_socket_service.hpp"

#include <boost/optional.hpp>
#include <boost/asio/basic_io_object.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>

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

    public:
        using native_handle_type = service_type::native_handle_type;
        using endpoint_type = boost::optional<std::string>;
        using message_flags = int;

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
         * receive a message for each buffer in the supplied sequence as part of a
         * multipar message before returning.
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
         * receive a message for each buffer in the supplied sequence before
         * returning.
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
         * receive a message for each buffer in the supplied sequence before
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
         * receive a message for each buffer in the supplied sequence before
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
            get_service().async_receive(implementation, buffers,
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
    };
}
#endif // AZIOMQ_IO_SOCKET_HPP_
