/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#include "../aziomq/thread.hpp"

#include <boost/format.hpp>

namespace aziomq {
namespace thread {

std::string forked_thread::concept::get_uri() {
    static unsigned long id = 0;
    return boost::str(boost::format("inproc://aziomq-pipe-%1%") % ++id);
}
} // namespace pipe
} // namespace aziomq
