add_executable(multiple_sockets main.cpp)
target_link_libraries(multiple_sockets ${Boost_LIBRARIES}
                                       ${ZeroMQ_LIBRARIES}
                                       aziomq)
