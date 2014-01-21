/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef __AZIOMQ_TEST_COMMON_HPP__
#define __AZIOMQ_TEST_COMMON_HPP__
namespace detail {
    struct apply_concept {
        virtual ~apply_concept() { }
        virtual void apply(boost::asio::io_service & ios, aziomq::socket & socket, int flags) = 0;
    };

    template<typename F,
            typename M>
    struct apply_model : apply_concept {
        apply_model(F func, M msg) :
            data(std::move(func)),
            msg(std::move(msg)) { }

        virtual void apply(boost::asio::io_service & ios, aziomq::socket & socket, int flags) {
            data(ios, socket, std::move(msg), flags);
        }

        F data;
        M msg;
    };

    template<typename F1,
            typename F2,
            typename M>
    struct fixture {
        using ptr_t = std::shared_ptr<apply_concept>;

        fixture(F1 func1, F2 func2, const M & msg) :
            f1(std::make_shared<apply_model<F1, M>>(std::move(func1), msg)),
            f2(std::make_shared<apply_model<F2, M>>(std::move(func2), msg)) { }

        void apply(std::string uri, int snd_flags, int rcv_flags) {
            std::thread t1(do_receiver, f1, uri, rcv_flags);
            std::thread t2(do_sender, f2, uri, snd_flags);
            t2.join();
            t1.join();
        }

        static void do_sender(ptr_t p, std::string uri, int flags) {
            boost::asio::io_service io_service;
            aziomq::socket socket(io_service, ZMQ_PAIR);
            socket.connect(uri);
            p->apply(io_service, socket, flags);
        }

        static void do_receiver(ptr_t p, std::string uri, int flags) {
            boost::asio::io_service io_service;
            aziomq::socket socket(io_service, ZMQ_PAIR);
            socket.bind(uri);
            p->apply(io_service, socket, flags);
        }

        ptr_t f1;
        ptr_t f2;
    };
} // namespace detail

template<typename F1,
         typename F2,
         typename M>
void apply_test(F1 func1, F2 func2, const M & msg, int snd_flags = 0, int rcv_flags = 0) {
    std::string uri{ "inproc://testing" };
    detail::fixture<F1, F2, M> f(std::move(func1), std::move(func2), msg);
    f.apply(uri, snd_flags, rcv_flags);
}
#endif // __AZIOMQ_TEST_COMMON_HPP__
