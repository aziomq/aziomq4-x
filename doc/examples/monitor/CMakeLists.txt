add_executable(aziomq_monitor main.cpp)
target_link_libraries(aziomq_monitor ${Boost_LIBRARIES}
                                   ${ZeroMQ_LIBRARIES}
                                   aziomq)
