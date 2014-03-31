add_executable(aziomq_proxy main.cpp)
target_link_libraries(aziomq_proxy ${Boost_LIBRARIES}
                                   ${ZeroMQ_LIBRARIES}
                                   aziomq)
