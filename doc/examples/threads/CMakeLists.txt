add_executable(aziomq_threads main.cpp)
target_link_libraries(aziomq_threads ${Boost_LIBRARIES}
                                     ${ZeroMQ_LIBRARIES}
                                     aziomq)

