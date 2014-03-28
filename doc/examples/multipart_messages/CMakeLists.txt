add_executable(multipart_messages main.cpp)
target_link_libraries(multipart_messages ${Boost_LIBRARIES}
                                         ${ZeroMQ_LIBRARIES}
                                         aziomq)
