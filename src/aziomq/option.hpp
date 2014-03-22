/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_OPTION_HPP_
#define AZIOMQ_OPTION_HPP_

#include <zmq.h>
#include <boost/asio/buffer.hpp>

#include <vector>
#include <string>

namespace aziomq { namespace opt {
    template<typename T, int N>
    struct opt_base {
        T value;

        opt_base() = default;
        opt_base(T v) : value(std::move(v)) { }

        int name() const { return N; }
        const void* data() const { return reinterpret_cast<const void*>(&value); }
        void* data() { return reinterpret_cast<void*>(&value); }
        size_t size() const { return sizeof(T); }

        void set(const T & v) { value = v; }

        operator T() const { return value; }
    };

    struct opt_nop_resize {
        void resize(size_t) { }
    };

    struct type : opt_base<int, ZMQ_TYPE>, opt_nop_resize {
        type(int v = -1) : opt_base(v) { }
    };

    struct io_threads : opt_base<int, ZMQ_IO_THREADS>, opt_nop_resize {
        io_threads(int v = 1) : opt_base(v) { }
    };

    struct max_sockets : opt_base<int, ZMQ_MAX_SOCKETS>, opt_nop_resize {
        max_sockets(int v = 1024) : opt_base(v) { }
    };

    struct ipv6 : opt_base<int, ZMQ_IPV6>, opt_nop_resize {
        ipv6(int v = 0) : opt_base(v) { }
    };

    struct rcv_more : opt_base<int, ZMQ_RCVMORE>, opt_nop_resize {
        rcv_more() : opt_base(0) { }
    };

    struct rcv_hwm : opt_base<int, ZMQ_RCVHWM>, opt_nop_resize {
        rcv_hwm(int v = 0) : opt_base(v) { }
    };

    struct snd_hwm : opt_base<int, ZMQ_SNDHWM>, opt_nop_resize {
        snd_hwm(int v = 0) : opt_base{ v } { }
    };

    struct affinity : opt_base<uint64_t, ZMQ_AFFINITY>, opt_nop_resize {
        affinity(uint64_t v = 0) : opt_base{ v } { }
    };

    struct rate : opt_base<int, ZMQ_RATE>, opt_nop_resize {
        rate(int v = 0) : opt_base{ v } { }
    };

    struct recovery_ivl : opt_base<int, ZMQ_RECOVERY_IVL>, opt_nop_resize {
        recovery_ivl(int v = 0) : opt_base{ v } { }
    };

    struct snd_buf : opt_base<int, ZMQ_SNDBUF>, opt_nop_resize {
        snd_buf(int v = 0) : opt_base{ v } { }
    };

    struct rcv_buf : opt_base<int, ZMQ_SNDBUF>, opt_nop_resize {
        rcv_buf(int v = 0) : opt_base{ v } { }
    };

    struct linger : opt_base<int, ZMQ_LINGER>, opt_nop_resize {
        linger(int v = 0) : opt_base{ v } { }
    };

    struct reconnect_ivl : opt_base<int, ZMQ_RECONNECT_IVL>, opt_nop_resize {
        reconnect_ivl(int v = 0) : opt_base{ v } { }
    };

    struct reconnect_ivl_max : opt_base<int, ZMQ_RECONNECT_IVL_MAX>, opt_nop_resize {
        reconnect_ivl_max(int v = 0) : opt_base{ v } { }
    };

    struct backlog : opt_base<int, ZMQ_BACKLOG>, opt_nop_resize {
        backlog(int v = 0) : opt_base{ v } { }
    };

    struct max_msgsize : opt_base<int, ZMQ_MAXMSGSIZE>, opt_nop_resize {
        max_msgsize(int v = 0) : opt_base{ v } { }
    };

    struct multicast_hops : opt_base<int, ZMQ_MULTICAST_HOPS>, opt_nop_resize {
        multicast_hops(int v = 0) : opt_base{ v } { }
    };

    struct rcv_timeo : opt_base<int, ZMQ_RCVTIMEO>, opt_nop_resize {
        rcv_timeo(int v = 0) : opt_base{ v } { }
    };

    struct snd_timeo : opt_base<int, ZMQ_RCVTIMEO>, opt_nop_resize {
        snd_timeo(int v = 0) : opt_base{ v } { }
    };

    struct ipv4_only : opt_base<int, ZMQ_IPV4ONLY>, opt_nop_resize {
        ipv4_only(int v = 0) : opt_base{ v } { }
    };

    struct immediate : opt_base<int, ZMQ_IMMEDIATE>, opt_nop_resize {
        immediate(int v = 0) : opt_base{ v } { }
    };

    struct router_mandatory : opt_base<int, ZMQ_ROUTER_MANDATORY>, opt_nop_resize {
        router_mandatory(int v = 0) : opt_base{ v } { }
    };

    struct router_raw : opt_base<int, ZMQ_ROUTER_RAW>, opt_nop_resize {
        router_raw(int v = 0) : opt_base{ v } { }
    };

    struct probe_router : opt_base<int, ZMQ_PROBE_ROUTER>, opt_nop_resize {
        probe_router(int v = 0) : opt_base{ v } { }
    };

    struct xpub_verbose : opt_base<int, ZMQ_XPUB_VERBOSE>, opt_nop_resize {
        xpub_verbose(int v = 0) : opt_base{ v } { }
    };

    struct req_correlate : opt_base<int, ZMQ_REQ_CORRELATE>, opt_nop_resize {
        req_correlate (int v = 0) : opt_base{ v } { }
    };

    struct req_relaxed : opt_base<int, ZMQ_REQ_RELAXED>, opt_nop_resize {
        req_relaxed(int v = 0) : opt_base{ v } { }
    };

    struct tcp_keepalive : opt_base<int, ZMQ_TCP_KEEPALIVE>, opt_nop_resize {
        tcp_keepalive(int v = 0) : opt_base{ v } { }
    };

    struct tcp_keepalive_idle : opt_base<int, ZMQ_TCP_KEEPALIVE_IDLE>, opt_nop_resize {
        tcp_keepalive_idle(int v = 0) : opt_base{ v } { }
    };

    struct tcp_keepalive_cnt : opt_base<int, ZMQ_TCP_KEEPALIVE_CNT>, opt_nop_resize {
        tcp_keepalive_cnt(int v = 0) : opt_base{ v } { }
    };

    struct tcp_keepalive_intvl : opt_base<int, ZMQ_TCP_KEEPALIVE_INTVL>, opt_nop_resize {
        tcp_keepalive_intvl(int v = 0) : opt_base{ v } { }
    };

    struct plain_server : opt_base<int, ZMQ_PLAIN_SERVER>, opt_nop_resize {
        plain_server(int v = 0) : opt_base{ v } { }
    };

    struct curve_server : opt_base<int, ZMQ_CURVE_SERVER>, opt_nop_resize {
        curve_server(int v = 0) : opt_base{ v } { }
    };

    struct conflate : opt_base<int, ZMQ_CONFLATE>, opt_nop_resize {
        conflate(int v = 0) : opt_base{ v } { }
    };

    //struct : opt_base<int, >, opt_nop_resize {
//(int v = 0) : opt_base{ v } { }
    //};

    template<int N>
    struct opt_binary {
        using value_t = std::vector<uint8_t>;
        value_t value;

        opt_binary() = default;
        opt_binary(const void *pv, size_t size) :
            value(from_pv(pv, size)) { }

        int name() const { return N; }
        const void* data() const {
            return reinterpret_cast<const void*>(value.data());
        }
        void* data() {
            return reinterpret_cast<void*>(value.data());
        }

        size_t size() const { return value.size(); }
        void resize(size_t size) {
            value.resize(size);
        }

        operator value_t() const { return value; }

        static std::vector<uint8_t> from_pv(const void *pv, size_t size) {
            auto p = reinterpret_cast<const value_t::value_type*>(pv);
            if (!p) return value_t();
            return value_t(p, p + size);
        }
    };

    struct subscribe : opt_binary<ZMQ_SUBSCRIBE> {
        subscribe(void *pv, size_t size) : opt_binary{ pv, size } { }
        subscribe(const std::string & v) : opt_binary(v.data(), v.size()) { }
    };

    struct unsubscribe : opt_binary<ZMQ_UNSUBSCRIBE> {
        unsubscribe(void *pv, size_t size) : opt_binary{ pv, size } { }
        unsubscribe(const std::string & v) : opt_binary(v.data(), v.size()) { }
    };

    struct identity : opt_binary<ZMQ_IDENTITY> {
        identity(void *pv, size_t size) : opt_binary{ pv, size } { }
    };

    struct tcp_accept_filter : opt_binary<ZMQ_TCP_ACCEPT_FILTER> {
        tcp_accept_filter(void *pv, size_t size) : opt_binary{ pv, size } { }
    };

    template<int N>
    struct opt_string {
        using value_t = std::vector<char>;
        value_t value;

        opt_string() = default;
        opt_string(const std::string & str) :
            value(std::begin(str), std::end(str)) { }

        int name() const { return N; }
        const void* data() const {
            return reinterpret_cast<const void*>(value.data());
        }
        void* data() {
            return reinterpret_cast<void*>(value.data());
        }

        size_t size() const { return value.size(); }
        void resize(size_t size) {
            value.resize(size);
        }

        operator value_t() const {
            std::string(std::begin(value), std::end(value));
        }
    };

    struct plain_username : opt_string<ZMQ_PLAIN_USERNAME> {
        plain_username(const std::string & v) : opt_string{ v } { }
    };

    struct plain_password : opt_string<ZMQ_PLAIN_PASSWORD> {
        plain_password(const std::string & v) : opt_string{ v } { }
    };

    struct zap_domain : opt_string<ZMQ_ZAP_DOMAIN> {
        zap_domain(const std::string & v) : opt_string{ v } { }
    };

    template<int N>
    struct opt_curve_key : opt_string<N> {
        opt_curve_key(const std::string & v) : opt_string<N>{ v } { }
        opt_curve_key(void *pv, size_t size) :
            opt_string<N>{ z85string(pv, size) } { }

        static std::string z85string(void *pv, size_t size) {
            std::vector<char> v;
            v.reserve(size * 1.25 + 1);
            zmq_z85_encode(v.data(), reinterpret_cast<uint8_t*>(pv), size);
            return std::string(v.data());
        }
    };

    struct curve_publickey : opt_curve_key<ZMQ_CURVE_PUBLICKEY> {
        curve_publickey(const std::string & v) : opt_curve_key{ v } { }
        curve_publickey(void *pv, size_t size) :
            opt_curve_key{ pv, size } { }
    };

    struct curve_privatekey : opt_curve_key<ZMQ_CURVE_PUBLICKEY> {
        curve_privatekey(const std::string & v) : opt_curve_key{ v } { }
        curve_privatekey(void *pv, size_t size) :
            opt_curve_key{ pv, size } { }
    };
} }

#endif // AZIOMQ_OPTION_HPP_
