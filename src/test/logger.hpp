/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef __AZIOMQ_TEST_LOGGER_HPP__
#define __AZIOMQ_TEST_LOGGER_HPP__

#include "../aziomq/detail/tracked_op.hpp"

#define INIT_SYNC_LOGGER(x) \
    aziomq::detail::tracked_op::init((x))

#define SYNC_LOG(x) \
    aziomq::detail::tracked_op::write_log((x))
#endif // __AZIOMQ_TEST_LOGGER_HPP__
