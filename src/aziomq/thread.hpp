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
#include "util/expected.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/exception/info_tuple.hpp>
#include <boost/exception/error_info.hpp>
#include <boost/exception/info.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <stdexcept>

namespace aziomq {
namespace thread {
    using is_alive = opt::peer::is_alive;
    using detached = opt::peer::detached;
    using last_error = opt::peer::last_error;
    using start = opt::peer::start;

    namespace error {
        using thread_name = boost::error_info<struct tag_thread_name, std::string>;
        using last_error = boost::error_info<struct tag_last_error, boost::system::error_code>;

        struct shout_failure : virtual boost::exception { };
    };
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
            return make_pipe(peer, false, std::bind(std::forward<Function>(f),
                                                    std::placeholders::_1,
                                                    std::forward<Args>(args)...));
        }

        struct defer_start_t { };
        template<typename Function, typename...Args>
        socket operator()(boost::asio::io_service & peer, defer_start_t, Function && f, Args&&... args) const {
            return make_pipe(peer, true, std::bind(std::forward<Function>(f),
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

            void run(socket s) override { data_(std::move(s)); }
        };

        struct handler {
            weak_ptr p_;
            bool defer_start_;

            handler(weak_ptr p, bool defer_start)
                : p_(std::move(p))
                , defer_start_(defer_start)
            { }

            handler(handler && rhs) = default;
            handler & operator=(handler && rhs) = default;

            ~handler() {
                if (auto p = p_.lock()) {
                    try { p->stop(); }
                    catch (...) { }
                }
            }

            void on_install() {
                if (defer_start_) return;

                if (auto p = p_.lock())
                    p->run();
            }

            void on_remove() {
                if (defer_start_) return;

                if (auto p = p_.lock())
                    p->stop();
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
                    case start::static_name::value :
                        {
                            auto v = reinterpret_cast<start::value_t const*>(opt.data());
                            if (*v && defer_start_) {
                                defer_start_ = false;
                                if (auto p = p_.lock())
                                    p->run();
                                else
                                    ec = make_error_code(boost::system::errc::interrupted);
                            }
                        }
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
                    case start::static_name::value :
                        {
                            auto v = reinterpret_cast<start::value_t*>(opt.data());
                            if (auto p = p_.lock())
                                *v = defer_start_ && !p->is_stopped();
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
        static socket make_pipe(boost::asio::io_service & peer, bool defer_start, T&& data) {
            auto p = std::make_shared<model<T>>(std::forward<T>(data));
            auto res = p->peer_socket(peer);
            res.associate_handler(handler(std::move(p), defer_start));
            return res;
        }
    } fork { };

    class group {
        using container_type = boost::container::flat_map<std::string, socket>;

    public:
        using expected_socket_ref = util::expected<socket::ref>;
        using expected_socket = util::expected<socket>;
        using expected_size = util::expected<size_t>;
        using const_iterator = container_type::const_iterator;

        group(boost::asio::io_service & io_service)
            : io_service_(io_service)
        { }

        group(group const&) = delete;
        group& operator=(group const&) = delete;

        template<typename Function,
                 typename... Args>
        expected_socket_ref create_thread(std::string name, Function && f, Args&&... args) {
            auto s = fork(io_service_, forked_thread::defer_start_t(), std::forward<Function>(f), std::forward<Args>(args)...);
            auto ib = sockets_.insert(std::make_pair(std::move(name), std::move(s)));
            auto& res = ib.first->second;
            if (ib.second)
                res.set_option(start());
            return std::ref(res);
        }

        expected_socket remove_thread(std::string const& name) {
            auto it = sockets_.find(name);
            if (it == std::end(sockets_))
                return expected_socket::from_exception(std::runtime_error("not found"));
            expected_socket res(std::move(it->second));
            sockets_.erase(it);
            return res;
        }

        // remove thread and send it a (presumably final) message
        template<typename ConstBufferSequence>
        expected_socket remove_thread(std::string const& name,
                                      ConstBufferSequence const& buffers,
                                      socket::message_flags flags) {
            auto res = remove_thread(name);
            if (res.valid()) {
                auto& s = res.get();
                boost::system::error_code ec;
                s.send(buffers, flags, ec);
                if (ec)
                    return expected_socket::from_exception(boost::system::system_error(ec));
            }
            return res;
        }

        void erase_thread(std::string const& name) { sockets_.erase(name); }

        const_iterator begin() const { return std::begin(sockets_); }
        const_iterator end() const { return std::end(sockets_); }

        bool in_pool(std::string const& name) const {
            auto it = sockets_.find(name);
            return it != std::end(sockets_);
        }

        expected_socket_ref get(std::string const& name) const {
            auto it = sockets_.find(name);
            if (it != std::end(sockets_))
                return std::ref(const_cast<socket&>(it->second));
            return expected_socket_ref::from_exception(std::runtime_error("not found"));
        }

        template<typename ConstBufferSequence>
        expected_size whisper(std::string const& name,
                              ConstBufferSequence const& buffers,
                              socket::message_flags flags) {
            auto it = sockets_.find(name);
            if (it == std::end(sockets_))
                expected_size::from_exception(boost::system::system_error(make_error_code(boost::system::errc::no_link)));
            boost::system::error_code ec;
            auto res = it->second.send(buffers, flags, ec);
            if (ec)
                expected_size::from_exception(boost::system::system_error(ec));
            return res;
        }

        template<typename ConstBufferSequence>
        expected_size shout(ConstBufferSequence const& buffers,
                            socket::message_flags flags) {
            size_t res = 0;
            for(auto& s : sockets_) {
                boost::system::error_code ec;
                auto v = s.second.send(buffers, flags, ec);
                if (ec)
                    return expected_size::from_exception(
                                error::shout_failure()
                                    << error::thread_name(s.first)
                                    << error::last_error(ec));
                res += v;
            }
            return res;
        }

    private:
        boost::asio::io_service & io_service_;
        container_type sockets_;
    };
} // namespace pipe
} // namespace aziomq
#endif // AZIOMQ_IO_PIPE_HPP_

