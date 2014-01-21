# Find ZeroMQ Headers/Libs

# Variables
# ZMQROOT - set this to a location where ZeroMQ may be found
#
# ZMQ_FOUND - True of ZeroMQ found
# ZMQ_INCLUDE_DIRS - Location of ZeroMQ includes
# ZMQ_LIBRARIS - ZeroMQ libraries

include(FindPackageHandleStandardArgs)

if ("$ENV{ZMQ_ROOT}" STREQUAL "")
    find_path(_ZeroMQ_ROOT NAMES include/zmq.h)
else()
    set(_ZeroMQ_ROOT "$ENV{ZMQ_ROOT}")
endif()

find_path(ZeroMQ_INCLUDE_DIRS NAMES zmq.h HINTS ${_ZeroMQ_ROOT}/include)

set(_ZeroMQ_H ${ZeroMQ_INCLUDE_DIRS}/zmq.h)

find_library(ZeroMQ_LIBRARIES NAMES zmq HINTS ${_ZeroMQ_ROOT}/lib)

function(_zmqver_EXTRACT _ZeroMQ_VER_COMPONENT _ZeroMQ_VER_OUTPUT)
    execute_process(
        COMMAND grep "#define ${_ZMQ_VER_COMPONENT}"
        COMMAND cut -d\  -f3
        RESULT_VARIABLE _zmqver_RESULT
        OUTPUT_VARIABLE _zmqver_OUTPUT
        INPUT_FILE ${_ZeroMQ_H}
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(${_ZeroMQ_VER_OUTPUT} ${_zmqver_OUTPUT} PARENT_SCOPE)
endfunction()

_zmqver_EXTRACT("ZMQ_VERSION_MAJOR" ZeroMQ_VERSION_MAJOR)
_zmqver_EXTRACT("ZMQ_VERSION_MINOR" ZeroMQ_VERSION_MINOR)

set(ZeroMQ_FIND_VERSION_EXACT "${ZMQ_VERSION_MAJOR}.${ZMQ_VERSION_MINOR}")
find_package_handle_standard_args(ZeroMQ FOUND_VAR ZeroMQ_FOUND
                                      REQUIRED_VARS ZeroMQ_INCLUDE_DIRS ZeroMQ_LIBRARIES
                                      VERSION_VAR ZeroMQ_VERSION)

if (ZeroMQ_FOUND)
    mark_as_advanced(ZeroMQ_FIND_VERSION_EXACT ZeroMQ_VERSION ZeroMQ_INCLUDE_DIRS ZeroMQ_LIBRARIES)
endif()
