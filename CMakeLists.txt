cmake_minimum_required(VERSION 2.8)
project(aziomq)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DAZIOMQ_ENABLE_HANDLER_TRACKING")

if(USE_LIBCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lc++abi")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lc++abi")
    set(ADDITIONAL_LIBS "pthread")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/config")

find_package(Boost 1.53 COMPONENTS system log REQUIRED)
find_package(ZeroMQ 4.0 REQUIRED)

include_directories(${Boost_INCLUDE_DIRS}
                    ${ZeroMQ_INCLUDE_DIRS})

enable_testing()
add_subdirectory(src)
