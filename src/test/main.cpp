/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#include "logger.hpp"
//#include "shared_context.hpp"
#include "single_message_tests.hpp"
#include "multi_message_tests.hpp"

#include "../aziomq/error.hpp"
#include "../aziomq/option.hpp"
#include "../aziomq/io_service.hpp"
#include "../aziomq/socket.hpp"

#include <zmq.h>
#include <boost/asio/buffer.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/test/included/prg_exec_monitor.hpp>

#include <iostream>
#include <array>
#include <thread>
#include <stdexcept>
#include <algorithm>
#include <iostream>

int cpp_main(int argc, char **argv) {
    INIT_SYNC_LOGGER(std::cout);

    single_message_tests::apply();
    multi_message_tests::apply();
    return 0;
}
