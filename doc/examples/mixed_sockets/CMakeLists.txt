add_executable(mixed_sockets main.cpp)
target_link_libraries(mixed_sockets ${Boost_LIBRARIES}
                                    ${ZeroMQ_LIBRARIES}
                                    aziomq)
