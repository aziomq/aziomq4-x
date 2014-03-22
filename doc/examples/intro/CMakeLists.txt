add_executable(aziomq_intro main.cpp)
target_link_libraries(aziomq_intro ${Boost_LIBRARIES}
                                   ${ZeroMQ_LIBRARIES}
                                   aziomq)
