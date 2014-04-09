/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is an implementation of Andrei Alexandrescu's Expected<T> type
    from the "Systematic Error Handling in C++" talk given at C++ And
    Beyond 2012
*/
#ifndef AZIOMQ_EXPECTED_HPP_
#define AZIOMQ_EXPECTED_HPP_

#include <boost/assert.hpp>
#include <exception>
#include <utility>
#include <typeinfo>
#include <stdexcept>
#include <cassert>

namespace aziomq { namespace util {
// define AZIOMQ_LOG_UNCHECKED *BEFORE* including expected.hpp to forward declare the following
// function to be called any time an exception is present and unchecked in an expected<T>
// when it's destructor is called
#ifdef AZIOMQ_LOG_UNCHECKED
    void log_expected_unchecked(std::exception_ptr err);
#endif

template<class T>
class expected {
    union {
        T val_;
        std::exception_ptr err_;
    };
    bool valid_;
    mutable bool unchecked_;

    expected() { }
public:
    expected(const T & rhs) : val_(rhs), valid_(true), unchecked_(false) { }
    expected(T && rhs) : val_(std::move(rhs)), valid_(true), unchecked_(false) { }
    expected(const expected & rhs) :
        valid_(rhs.valid_),
        unchecked_(rhs.unchecked_) {
            if (rhs.valid_)
                new(&val_) T(rhs.val_);
            else
                new(&err_) std::exception_ptr(rhs.err_);
        }

    expected(expected && rhs) :
        valid_(rhs.valid_),
        unchecked_(rhs.unchecked_) {
            if (rhs.valid_)
                new(&val_) T(std::move(rhs.val_));
            else
                new(&err_) std::exception_ptr(std::move(rhs.err_));
            rhs.unchecked_ = false;
        }

    ~expected() {
#ifdef AZIOMQ_LOG_UNCHECKED
        if (unchecked_)
            log_expected_unchecked(err_);
#else
        BOOST_ASSERT_MSG(!unchecked_, "error result not checked");
#endif
    }

    void swap(expected& rhs) {
        if (valid_) {
            if (rhs.valid_) {
                std::swap(val_, rhs.val_);
            } else {
                auto t = std::move(rhs.err_);
                new(&rhs.val_) T(std::move(val_));
                new(&err_) std::exception_ptr(t);
                std::swap(valid_, rhs.valid_);
                std::swap(unchecked_, rhs.unchecked_);
            }
        } else {
            if (rhs.valid_) {
                rhs.swap(*this);
            } else {
                std::swap(err_, rhs.err_);
                std::swap(valid_, rhs.valid_);
                std::swap(unchecked_, rhs.unchecked_);
            }
        }
    }

    template<class E>
    static expected<T> from_exception(const E & exception) {
        if (typeid(exception) != typeid(E))
            throw std::invalid_argument("slicing detected");
        return from_exception(std::make_exception_ptr(exception));
    }

    static expected<T> from_exception(std::exception_ptr p) {
        expected<T> res;
        res.valid_ = false;
        res.unchecked_ = true;
        new(&res.err_) std::exception_ptr(std::move(p));
        return res;
    }

    static expected<T> from_exception() {
        return from_exception(std::current_exception());
    }

    bool valid() const {
        unchecked_ = false;
        return valid_;
    }

    const T & get() const {
        unchecked_ = false;
        if (!valid_) std::rethrow_exception(err_);
        return val_;
    }

    T & get() {
        unchecked_ = false;
        if (!valid_) std::rethrow_exception(err_);
        return val_;
    }

    template<class E>
    bool has_exception() const {
        unchecked_ = false;
        try {
            if (!valid_) std::rethrow_exception(err_);
        } catch (const E &) {
            return true;
        } catch (...) { }
        return false;
    }
};
} // namespace util
} // namespace aziomq
#endif // AZIOMQ_EXPECTED_HPP_
