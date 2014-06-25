/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_IO_PIPE_HPP_
#define AZIOMQ_IO_PIPE_HPP_

#include "socket.hpp"
#include "option.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace aziomq {
namespace thread {
    using is_alive = opt::peer::is_alive;
    using detached = opt::peer::detached;
    using last_error = opt::peer::last_error;

    /** \brief create a thread bound to one end of a pair of inproc sockets
     *  \param peer io_service to associate the peer (caller) end of the pipe
     *  \param f Function accepting a socket as it's first parameter and a
     *           number of additional args
     *  \returns peer socket
     *
     *  \remark The newly created thread will be a std::thread, and will receive the
     *  'server' end of the pipe as it's first argument.  This thread will be
     *  attached to the lifetime of the returned socket and will run until it is destroyed.
     *
     *  \remark Each forked thread has an associated io_service and the supplied socket
     *  will be created on this io_service. The thread function may access this by calling
     *  get_io_service() on the supplied socket.
     *
     *  \remark The associated io_service is configured to stop the spawned thread on
     *  SIG_KILL and SIG_TERM.
     *
     *  \remark Termination:
     *      "well behaved" threads should ultimately call run() on the io_service. This allows
     *      the 'client' end of the socket's lifetime to cleanly signal termination of the thread.
     *      If for some reason, this is not possible, the caller should set the 'detached' option
     *      on the 'client' socket. This detaches the associated thread from the client socket
     *      so that it will not be joined at destuction time. It is then up to the caller to
     *      work out the termination signal for the background thread; for instance by sending a
     *      termination message.
     *
     *      Also note, the default signal handling for the background thread is desinged to call
     *      stop() on the associated io_service, so not calling run() in your handler means you
     *      are responsible for catching these signals in some other way.
     *
     *  \remark This is similar in concept to the CZMQ zthread_fork API, except that lifetime is
     *  controled by the returned socket, not a separate zctx_t instance.
     */
    constexpr struct forked_thread {
        template<typename Function, typename...Args>
        socket operator()(boost::asio::io_service & peer, Function && f, Args&&... args) const {
            return make_pipe(peer, std::bind(std::forward<Function>(f),
                                             std::placeholders::_1,
                                             std::forward<Args>(args)...));
        }

    private:
        using service_type = socket::service_type;
        struct concept
            : std::enable_shared_from_this<concept> {

            std::string uri_;
            boost::asio::io_service ios_;
            boost::asio::signal_set signals_;
            socket s_;

            std::thread t_;

            using lock_t = std::unique_lock<std::mutex>;
            mutable lock_t::mutex_type mtx_;
            mutable std::condition_variable cv_;

            // synchronized state
            bool ready_;
            bool stopped_;
            std::exception_ptr last_error_;

            concept()
                : uri_(get_uri())
                , signals_(ios_, SIGINT, SIGTERM)
                , s_(ios_, ZMQ_PAIR)
                , ready_(false)
                , stopped_(true)
            {
                s_.bind(uri_);
            }

            bool joinable() const { return t_.joinable(); }

            void stop() {
                if (joinable()) {
                    ios_.stop();
                    t_.join();
                }
            }

            void stopped() {
                lock_t lock(mtx_);
                stopped_ = true;
            }

            bool is_stopped() {
                lock_t lock(mtx_);
                return stopped_;
            }

            void ready() {
                {
                    lock_t lock(mtx_);
                    ready_ = true;
                }
                cv_.notify_all();
            }

            void detach() {
                if (!joinable()) return; // already detached

                lock_t lock(mtx_);
                cv_.wait(lock, [this] { return ready_; });
                t_.detach();
            }

            socket peer_socket(boost::asio::io_service & peer) {
                auto res = socket(peer, ZMQ_PAIR);
                res.connect(uri_);
                return res;
            }

            virtual void run(socket) = 0;

            void set_last_error(std::exception_ptr last_error) {
                lock_t lock(mtx_);
                last_error_ = last_error;
            }

            std::exception_ptr last_error() const {
                lock_t lock(mtx_);
                return last_error_;
            }

            void run() {
                signals_.async_wait([this](const boost::system::error_code &, int) {
                    ios_.stop();
                });

                auto p = shared_from_this();
                t_ = std::thread([p] {
                    p->ready();
                    try {
                        p->run(std::move(p->s_));
                    } catch (...) {
                        p->set_last_error(std::current_exception());
                    }
                    p->stopped();
                });
            }

            static std::string get_uri();
        };
        using ptr = std::shared_ptr<concept>;
        using weak_ptr = std::weak_ptr<concept>;

        template<typename T>
        struct model : concept {
            T data_;

            model(T data) : data_(std::move(data)) { }

            void run(socket s) { data_(std::move(s)); }
        };

        struct handler {
            weak_ptr p_;

            handler(weak_ptr p) : p_(std::move(p)) { }

            handler(handler && rhs) = default;
            handler & operator=(handler && rhs) = default;

            ~handler() {
                if (auto p = p_.lock()) {
                    try { p->stop(); }
                    catch (...) { }
                }
            }

            void on_install() {
                if (auto p = p_.lock())
                    p->run();
            }

            boost::system::error_code set_option(service_type::opt_concept const& opt,
                                                 boost::system::error_code & ec) {
                switch (opt.name()) {
                    case is_alive::static_name::value :
                        ec = make_error_code(boost::system::errc::no_protocol_option);
                        break;
                    case detached::static_name::value :
                        {
                            auto v = reinterpret_cast<detached::value_t const*>(opt.data());
                            if (*v) {
                                if (auto p = p_.lock())
                                    p->detach();
                                else
                                    ec = make_error_code(boost::system::errc::interrupted);
                            }
                        }
                        break;
                    case last_error::static_name::value :
                        ec = make_error_code(boost::system::errc::no_protocol_option);
                        break;
                    default:
                        ec = make_error_code(boost::system::errc::not_supported);
                        break;
                }
                return ec;
            }

            boost::system::error_code get_option(service_type::opt_concept & opt,
                                                 boost::system::error_code & ec) {
                switch (opt.name()) {
                    case is_alive::static_name::value :
                        {
                            auto v = reinterpret_cast<is_alive::value_t*>(opt.data());
                            if (auto p = p_.lock())
                                *v = boost::logic::tribool(p->is_stopped());
                            else
                                ec = make_error_code(boost::system::errc::interrupted);
                        }
                        break;
                    case detached::static_name::value :
                        {
                            auto v = reinterpret_cast<detached::value_t*>(opt.data());
                            if (auto p = p_.lock())
                                *v = p->joinable();
                            else
                                ec = make_error_code(boost::system::errc::interrupted);
                        }
                        break;
                    case last_error::static_name::value :
                        {
                            auto v = reinterpret_cast<last_error::value_t*>(opt.data());
                            if (auto p = p_.lock())
                                *v = p->last_error();
                            else
                                ec = make_error_code(boost::system::errc::interrupted);
                        }
                        break;
                    default:
                      ec = make_error_code(boost::system::errc::not_supported);
                      break;
                }
                return ec;
            }
        };

        template<typename T>
        static socket make_pipe(boost::asio::io_service & peer, T&& data) {
            auto p = std::make_shared<model<T>>(std::forward<T>(data));
            auto res = p->peer_socket(peer);
            res.associate_handler(handler(std::move(p)));
            return res;
        }
    } fork { };
} // namespace pipe
} // namespace aziomq
#endif // AZIOMQ_IO_PIPE_HPP_

